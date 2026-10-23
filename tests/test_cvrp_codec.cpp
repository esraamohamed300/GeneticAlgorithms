// Unit tests for the CVRP encoder/decoder (CVRPCodec).

#include "TestUtils.h"

#include "CVRPCodec.h"

#include <algorithm>
#include <random>
#include <string>

namespace {

// Every customer 1..n appears exactly once across the routes, and there are
// exactly K routes.
bool servesEveryCustomerOnce(const Routes& routes, const CVRPCodec& codec) {
    if (static_cast<int>(routes.size()) != codec.numVehicles()) {
        return false;
    }
    std::vector<int> all;
    for (const Route& r : routes) {
        all.insert(all.end(), r.begin(), r.end());
    }
    std::sort(all.begin(), all.end());
    if (static_cast<int>(all.size()) != codec.numCustomers()) {
        return false;
    }
    for (int i = 0; i < codec.numCustomers(); ++i) {
        if (all[static_cast<std::size_t>(i)] != i + 1) {
            return false;
        }
    }
    return true;
}

// Pads with empty routes up to K, like the codec does when decoding.
Routes padded(Routes routes, int vehicles) {
    routes.resize(static_cast<std::size_t>(vehicles));
    return routes;
}

}  // namespace

void testConstructionAndSizes() {
    CVRPCodec codec(7, 3);
    CHECK(codec.numCustomers() == 7);
    CHECK(codec.numVehicles() == 3);
    CHECK(codec.permutationLength() == 9);  // 7 customers + 2 separators
    CHECK(codec.bitsPerKey() == 8);         // default: max(8, ceil(log2 9))
    CHECK(codec.binaryLength() == 72);

    CHECK(codec.isCustomer(1) && codec.isCustomer(7));
    CHECK(!codec.isCustomer(0) && !codec.isCustomer(8));
    CHECK(codec.isSeparator(8) && codec.isSeparator(9));
    CHECK(!codec.isSeparator(7) && !codec.isSeparator(10));

    // Big instances automatically get wide enough keys.
    CVRPCodec big(1000, 20);
    CHECK(big.bitsPerKey() >= 10);  // 1019 values need >= 10 bits

    CHECK_THROWS(CVRPCodec(0, 3), std::invalid_argument);
    CHECK_THROWS(CVRPCodec(5, 0), std::invalid_argument);
    CHECK_THROWS(CVRPCodec(5, 2, 32), std::invalid_argument);
    CHECK_THROWS(CVRPCodec(5, 2, -1), std::invalid_argument);
}

// Minimum validation from the task: decode a sample chromosome into routes and
// check customers are represented according to the chosen model.
void testDecodeSampleInteger() {
    CVRPCodec codec(7, 3);  // separators are 8 and 9

    IntegerChromosome c({3, 1, 2, 8, 5, 4, 9, 6, 7}, 1, 9, true);
    Routes routes = codec.decode(c);

    CHECK(routes.size() == 3);
    CHECK((routes[0] == Route{3, 1, 2}));
    CHECK((routes[1] == Route{5, 4}));
    CHECK((routes[2] == Route{6, 7}));
    CHECK(servesEveryCustomerOnce(routes, codec));

    // Depot is implicit and added on request.
    CHECK((CVRPCodec::withDepot(routes[0]) == Route{0, 3, 1, 2, 0}));
    CHECK((CVRPCodec::withDepot(Route{}) == Route{0, 0}));
}

void testDecodeEmptyRoutes() {
    CVRPCodec codec(7, 3);

    // Separators adjacent -> middle vehicle unused.
    Routes a = codec.decode(IntegerChromosome({1, 2, 3, 8, 9, 4, 5, 6, 7}, 1, 9, true));
    CHECK((a[0] == Route{1, 2, 3}));
    CHECK(a[1].empty());
    CHECK((a[2] == Route{4, 5, 6, 7}));

    // Separators at both ends -> first and last vehicles unused.
    Routes b = codec.decode(IntegerChromosome({8, 1, 2, 3, 4, 5, 6, 7, 9}, 1, 9, true));
    CHECK(b[0].empty());
    CHECK((b[1] == Route{1, 2, 3, 4, 5, 6, 7}));
    CHECK(b[2].empty());

    // Separator order in the tour does not matter, only their positions.
    Routes c = codec.decode(IntegerChromosome({9, 1, 2, 8, 3, 4, 5, 6, 7}, 1, 9, true));
    CHECK(c[0].empty());
    CHECK((c[1] == Route{1, 2}));
    CHECK((c[2] == Route{3, 4, 5, 6, 7}));
}

void testSingleVehicleHasNoSeparators() {
    CVRPCodec codec(4, 1);
    CHECK(codec.permutationLength() == 4);

    Routes routes = codec.decode(IntegerChromosome({2, 4, 1, 3}, 1, 4, true));
    CHECK(routes.size() == 1);
    CHECK((routes[0] == Route{2, 4, 1, 3}));
}

void testDecodeFloatRandomKeys() {
    CVRPCodec codec(4, 2);  // L = 5, separator is 5
    // value:     1    2    3    4    5(sep)
    // key:      0.9  0.1  0.5  0.3  0.6
    // ascending order of keys: 2(0.1) 4(0.3) 3(0.5) 5(0.6) 1(0.9)
    FloatChromosome c({0.9, 0.1, 0.5, 0.3, 0.6}, 0.0, 1.0);
    Routes routes = codec.decode(c);
    CHECK((routes[0] == Route{2, 4, 3}));
    CHECK((routes[1] == Route{1}));
}

void testDecodeBinaryKeys() {
    CVRPCodec codec(4, 2, 4);  // 4 bits per key, L = 5 -> 20 bits
    CHECK(codec.binaryLength() == 20);

    BinaryChromosome c(std::vector<std::uint8_t>(20, 0));
    // keys (value -> key): 1->9, 2->1, 3->5, 4->3, 5(sep)->6
    c.encodeUnsigned(0 * 4, 4, 9);
    c.encodeUnsigned(1 * 4, 4, 1);
    c.encodeUnsigned(2 * 4, 4, 5);
    c.encodeUnsigned(3 * 4, 4, 3);
    c.encodeUnsigned(4 * 4, 4, 6);

    Routes routes = codec.decode(c);
    CHECK((routes[0] == Route{2, 4, 3}));
    CHECK((routes[1] == Route{1}));
}

void testBinaryTiesAreDeterministicAndValid() {
    CVRPCodec codec(5, 2);
    // All-zero bits -> every key equal -> ties broken by index.
    BinaryChromosome zeros(std::vector<std::uint8_t>(codec.binaryLength(), 0));
    Routes routes = codec.decode(zeros);
    CHECK(servesEveryCustomerOnce(routes, codec));
    CHECK((routes[0] == Route{1, 2, 3, 4, 5}));  // separator (6) is last
    CHECK(routes[1].empty());
    CHECK(codec.decode(zeros) == routes);  // same input, same output
}

// Encode known routes, decode them back (all three representations).
void testEncodeDecodeRoundTripKnownRoutes() {
    CVRPCodec codec(7, 3);
    Routes routes = {{3, 1, 2}, {5, 4}, {6, 7}};

    IntegerChromosome ic = codec.encodeInteger(routes);
    CHECK(ic.isValid() && ic.isPermutation());
    CHECK(ic.toString() == "3 1 2 8 5 4 9 6 7");
    CHECK(codec.decode(ic) == routes);

    FloatChromosome fc = codec.encodeFloat(routes);
    CHECK(fc.isValid());
    CHECK(codec.decode(fc) == routes);

    BinaryChromosome bc = codec.encodeBinary(routes);
    CHECK(bc.isValid());
    CHECK(bc.size() == codec.binaryLength());
    CHECK(codec.decode(bc) == routes);
}

void testEncodeFewerRoutesThanVehicles() {
    CVRPCodec codec(5, 4);
    Routes routes = {{2, 1, 5}, {4, 3}};  // only 2 of 4 vehicles used

    Routes expected = padded(routes, 4);
    CHECK(codec.decode(codec.encodeInteger(routes)) == expected);
    CHECK(codec.decode(codec.encodeFloat(routes)) == expected);
    CHECK(codec.decode(codec.encodeBinary(routes)) == expected);

    // Empty route in the middle is preserved.
    Routes gap = {{1, 2}, {}, {3, 4, 5}};
    CHECK(codec.decode(codec.encodeInteger(gap)) == padded(gap, 4));
}

// Separators are interchangeable: only their POSITIONS matter, so different
// integer chromosomes can decode to the same routes. encode() always writes
// them in canonical order (n+1, n+2, ...).
void testSeparatorsAreInterchangeable() {
    CVRPCodec codec(7, 3);  // separators 8 and 9

    IntegerChromosome a({3, 1, 2, 8, 5, 4, 9, 6, 7}, 1, 9, true);
    IntegerChromosome b({3, 1, 2, 9, 5, 4, 8, 6, 7}, 1, 9, true);
    CHECK(a.genes() != b.genes());
    CHECK(codec.decode(a) == codec.decode(b));

    // Both normalise to the canonical chromosome after a decode/encode trip.
    CHECK(codec.encodeInteger(codec.decode(b)).genes() == a.genes());
}

// decode(encode(decode(x))) == decode(x) for lots of random chromosomes, and
// every decoded solution serves each customer exactly once.
void testRandomRoundTripAllRepresentations() {
    std::mt19937 rng(2024);

    struct Case { int customers; int vehicles; };
    for (Case cs : {Case{1, 1}, Case{5, 1}, Case{1, 4}, Case{7, 3},
                    Case{20, 5}, Case{60, 8}, Case{300, 12}}) {
        CVRPCodec codec(cs.customers, cs.vehicles);

        for (int trial = 0; trial < 25; ++trial) {
            IntegerChromosome ic = codec.createInteger(rng);
            CHECK(ic.isValid());
            Routes ri = codec.decode(ic);
            CHECK(servesEveryCustomerOnce(ri, codec));
            CHECK(codec.decode(codec.encodeInteger(ri)) == ri);

            FloatChromosome fc = codec.createFloat(rng);
            CHECK(fc.isValid());
            Routes rf = codec.decode(fc);
            CHECK(servesEveryCustomerOnce(rf, codec));
            CHECK(codec.decode(codec.encodeFloat(rf)) == rf);

            BinaryChromosome bc = codec.createBinary(rng);
            CHECK(bc.isValid());
            CHECK(bc.size() == codec.binaryLength());
            Routes rb = codec.decode(bc);
            CHECK(servesEveryCustomerOnce(rb, codec));
            CHECK(codec.decode(codec.encodeBinary(rb)) == rb);
        }
    }
}

// Float and binary chromosomes must stay decodable after any gene change, so
// multi-point crossover / mutation can never produce an undecodable child.
void testKeyRepresentationsAreAlwaysDecodable() {
    std::mt19937 rng(77);
    CVRPCodec codec(15, 4);

    FloatChromosome fc = codec.createFloat(rng);
    BinaryChromosome bc = codec.createBinary(rng);
    std::uniform_real_distribution<double> real(0.0, 1.0);

    for (int step = 0; step < 200; ++step) {
        fc[static_cast<std::size_t>(rng() % fc.size())] = real(rng);
        bc.flip(static_cast<std::size_t>(rng() % bc.size()));
        CHECK(servesEveryCustomerOnce(codec.decode(fc), codec));
        CHECK(servesEveryCustomerOnce(codec.decode(bc), codec));
    }
}

void testDecodeRejectsMalformedChromosomes() {
    CVRPCodec codec(7, 3);  // L = 9

    // Wrong length.
    CHECK_THROWS(codec.decode(IntegerChromosome({1, 2, 3}, 1, 3, true)),
                 std::invalid_argument);
    CHECK_THROWS(codec.decode(FloatChromosome({0.1, 0.2}, 0.0, 1.0)),
                 std::invalid_argument);
    CHECK_THROWS(codec.decode(BinaryChromosome({0, 1, 0})),
                 std::invalid_argument);

    // Right length but wrong bounds (e.g. built for a different instance).
    IntegerChromosome wrongBounds({1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 10, false);
    CHECK_THROWS(codec.decode(wrongBounds), std::invalid_argument);

    // Not a permutation (duplicate): corrupt a valid one through raw access.
    IntegerChromosome dup({1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 9, true);
    dup[1] = 1;
    CHECK_THROWS(codec.decode(dup), std::invalid_argument);

    // Float gene corrupted to NaN / out of range.
    std::mt19937 rng(5);
    FloatChromosome bad = codec.createFloat(rng);
    bad[0] = 2.0;
    CHECK_THROWS(codec.decode(bad), std::invalid_argument);
}

void testEncodeRejectsInvalidRoutes() {
    CVRPCodec codec(5, 2);

    CHECK_THROWS(codec.encodeInteger({{1, 2}, {3, 4}}),           // 5 missing
                 std::invalid_argument);
    CHECK_THROWS(codec.encodeInteger({{1, 2, 3}, {3, 4, 5}}),     // 3 twice
                 std::invalid_argument);
    CHECK_THROWS(codec.encodeInteger({{1, 2, 3}, {4, 6}}),        // id 6
                 std::invalid_argument);
    CHECK_THROWS(codec.encodeInteger({{0, 1, 2}, {3, 4, 5}}),     // depot id
                 std::invalid_argument);
    CHECK_THROWS(codec.encodeInteger({{1}, {2}, {3, 4, 5}}),      // 3 > K=2
                 std::invalid_argument);
    CHECK_THROWS(codec.encodeFloat({{1, 2}, {3, 4}}),
                 std::invalid_argument);
    CHECK_THROWS(codec.encodeBinary({{1, 2}, {3, 4}}),
                 std::invalid_argument);

    // Valid input is accepted.
    CHECK(codec.decode(codec.encodeInteger({{5, 1, 2, 3, 4}})) ==
          padded({{5, 1, 2, 3, 4}}, 2));
}

void testEncodeBinaryNeedsEnoughBits() {
    // L = 9 values need keys 0..8, which do not fit in 3 bits (max 7).
    CVRPCodec tooSmall(7, 3, 3);
    CHECK_THROWS(tooSmall.encodeBinary({{1, 2, 3}, {4, 5}, {6, 7}}),
                 std::invalid_argument);
    // 4 bits are enough.
    CVRPCodec ok(7, 3, 4);
    Routes r = {{1, 2, 3}, {4, 5}, {6, 7}};
    CHECK(ok.decode(ok.encodeBinary(r)) == r);
}

void testCreatedChromosomesWorkThroughBasePointer() {
    std::mt19937 rng(31);
    CVRPCodec codec(10, 3);

    std::unique_ptr<Chromosome> pop[] = {
        codec.createInteger(rng).clone(),
        codec.createFloat(rng).clone(),
        codec.createBinary(rng).clone(),
    };

    // A fitness/feasibility function only sees `const Chromosome&`; it has to
    // work out the representation itself, as the CVRP runner will.
    int decoded = 0;
    for (auto& p : pop) {
        if (auto* i = dynamic_cast<IntegerChromosome*>(p.get())) {
            decoded += servesEveryCustomerOnce(codec.decode(*i), codec);
        } else if (auto* f = dynamic_cast<FloatChromosome*>(p.get())) {
            decoded += servesEveryCustomerOnce(codec.decode(*f), codec);
        } else if (auto* b = dynamic_cast<BinaryChromosome*>(p.get())) {
            decoded += servesEveryCustomerOnce(codec.decode(*b), codec);
        }
    }
    CHECK(decoded == 3);
}

int main() {
    RUN_TEST(testConstructionAndSizes);
    RUN_TEST(testDecodeSampleInteger);
    RUN_TEST(testDecodeEmptyRoutes);
    RUN_TEST(testSingleVehicleHasNoSeparators);
    RUN_TEST(testDecodeFloatRandomKeys);
    RUN_TEST(testDecodeBinaryKeys);
    RUN_TEST(testBinaryTiesAreDeterministicAndValid);
    RUN_TEST(testEncodeDecodeRoundTripKnownRoutes);
    RUN_TEST(testEncodeFewerRoutesThanVehicles);
    RUN_TEST(testSeparatorsAreInterchangeable);
    RUN_TEST(testRandomRoundTripAllRepresentations);
    RUN_TEST(testKeyRepresentationsAreAlwaysDecodable);
    RUN_TEST(testDecodeRejectsMalformedChromosomes);
    RUN_TEST(testEncodeRejectsInvalidRoutes);
    RUN_TEST(testEncodeBinaryNeedsEnoughBits);
    RUN_TEST(testCreatedChromosomesWorkThroughBasePointer);

    return finishTests();
}
