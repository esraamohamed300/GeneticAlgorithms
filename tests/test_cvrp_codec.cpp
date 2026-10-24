

#include "TestUtils.h"

#include "CVRPCodec.h"

#include <algorithm>
#include <random>
#include <string>
using namespace std;


bool servesEveryCustomerOnce(const Routes& routes, const CVRPCodec& codec) {
    if (static_cast<int>(routes.size()) != codec.numVehicles()) {
        return false;
    }
    vector<int> all;
    for (const Route& r : routes) {
        all.insert(all.end(), r.begin(), r.end());
    }
    sort(all.begin(), all.end());
    if (static_cast<int>(all.size()) != codec.numCustomers()) {
        return false;
    }
    for (int i = 0; i < codec.numCustomers(); ++i) {
        if (all[static_cast<int>(i)] != i + 1) {
            return false;
        }
    }
    return true;
}


Routes padded(Routes routes, int vehicles) {
    routes.resize(static_cast<int>(vehicles));
    return routes;
}

void testConstructionAndSizes() {
    CVRPCodec codec(7, 3);
    CHECK(codec.numCustomers() == 7);
    CHECK(codec.numVehicles() == 3);
    CHECK(codec.permutationLength() == 9);  
    CHECK(codec.bitsPerKey() == 8);         
    CHECK(codec.binaryLength() == 72);

    CHECK(codec.isCustomer(1) && codec.isCustomer(7));
    CHECK(!codec.isCustomer(0) && !codec.isCustomer(8));
    CHECK(codec.isSeparator(8) && codec.isSeparator(9));
    CHECK(!codec.isSeparator(7) && !codec.isSeparator(10));

    
    CVRPCodec big(1000, 20);
    CHECK(big.bitsPerKey() >= 10);  

    CHECK_THROWS(CVRPCodec(0, 3), invalid_argument);
    CHECK_THROWS(CVRPCodec(5, 0), invalid_argument);
    CHECK_THROWS(CVRPCodec(5, 2, 32), invalid_argument);
    CHECK_THROWS(CVRPCodec(5, 2, -1), invalid_argument);
}


void testDecodeSampleInteger() {
    CVRPCodec codec(7, 3);  

    IntegerChromosome c({3, 1, 2, 8, 5, 4, 9, 6, 7}, 1, 9, true);
    Routes routes = codec.decode(c);

    CHECK(routes.size() == 3);
    CHECK((routes[0] == Route{3, 1, 2}));
    CHECK((routes[1] == Route{5, 4}));
    CHECK((routes[2] == Route{6, 7}));
    CHECK(servesEveryCustomerOnce(routes, codec));

    
    CHECK((CVRPCodec::withDepot(routes[0]) == Route{0, 3, 1, 2, 0}));
    CHECK((CVRPCodec::withDepot(Route{}) == Route{0, 0}));
}

void testDecodeEmptyRoutes() {
    CVRPCodec codec(7, 3);

    Routes a = codec.decode(IntegerChromosome({1, 2, 3, 8, 9, 4, 5, 6, 7}, 1, 9, true));
    CHECK((a[0] == Route{1, 2, 3}));
    CHECK(a[1].empty());
    CHECK((a[2] == Route{4, 5, 6, 7}));

    
    Routes b = codec.decode(IntegerChromosome({8, 1, 2, 3, 4, 5, 6, 7, 9}, 1, 9, true));
    CHECK(b[0].empty());
    CHECK((b[1] == Route{1, 2, 3, 4, 5, 6, 7}));
    CHECK(b[2].empty());

   
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
    CVRPCodec codec(4, 2);  

    FloatChromosome c({0.9, 0.1, 0.5, 0.3, 0.6}, 0.0, 1.0);
    Routes routes = codec.decode(c);
    CHECK((routes[0] == Route{2, 4, 3}));
    CHECK((routes[1] == Route{1}));
}

void testDecodeBinaryKeys() {
    CVRPCodec codec(4, 2, 4);  
    CHECK(codec.binaryLength() == 20);

    BinaryChromosome c(vector<uint8_t>(20, 0));
   
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
    
    BinaryChromosome zeros(vector<uint8_t>(codec.binaryLength(), 0));
    Routes routes = codec.decode(zeros);
    CHECK(servesEveryCustomerOnce(routes, codec));
    CHECK((routes[0] == Route{1, 2, 3, 4, 5}));  
    CHECK(routes[1].empty());
    CHECK(codec.decode(zeros) == routes);  
}


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
    Routes routes = {{2, 1, 5}, {4, 3}};  

    Routes expected = padded(routes, 4);
    CHECK(codec.decode(codec.encodeInteger(routes)) == expected);
    CHECK(codec.decode(codec.encodeFloat(routes)) == expected);
    CHECK(codec.decode(codec.encodeBinary(routes)) == expected);

  
    Routes gap = {{1, 2}, {}, {3, 4, 5}};
    CHECK(codec.decode(codec.encodeInteger(gap)) == padded(gap, 4));
}


void testSeparatorsAreInterchangeable() {
    CVRPCodec codec(7, 3);  

    IntegerChromosome a({3, 1, 2, 8, 5, 4, 9, 6, 7}, 1, 9, true);
    IntegerChromosome b({3, 1, 2, 9, 5, 4, 8, 6, 7}, 1, 9, true);
    CHECK(a.genes() != b.genes());
    CHECK(codec.decode(a) == codec.decode(b));

    CHECK(codec.encodeInteger(codec.decode(b)).genes() == a.genes());
}


void testRandomRoundTripAllRepresentations() {
    mt19937 rng(2024);

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


void testKeyRepresentationsAreAlwaysDecodable() {
    mt19937 rng(77);
    CVRPCodec codec(15, 4);

    FloatChromosome fc = codec.createFloat(rng);
    BinaryChromosome bc = codec.createBinary(rng);
    uniform_real_distribution<double> real(0.0, 1.0);

    for (int step = 0; step < 200; ++step) {
        fc[static_cast<int>(rng() % fc.size())] = real(rng);
        bc.flip(static_cast<int>(rng() % bc.size()));
        CHECK(servesEveryCustomerOnce(codec.decode(fc), codec));
        CHECK(servesEveryCustomerOnce(codec.decode(bc), codec));
    }
}

void testDecodeRejectsMalformedChromosomes() {
    CVRPCodec codec(7, 3); 

    CHECK_THROWS(codec.decode(IntegerChromosome({1, 2, 3}, 1, 3, true)),
                 invalid_argument);
    CHECK_THROWS(codec.decode(FloatChromosome({0.1, 0.2}, 0.0, 1.0)),
                 invalid_argument);
    CHECK_THROWS(codec.decode(BinaryChromosome({0, 1, 0})),
                 invalid_argument);

    IntegerChromosome wrongBounds({1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 10, false);
    CHECK_THROWS(codec.decode(wrongBounds), invalid_argument);

    IntegerChromosome dup({1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 9, true);
    dup[1] = 1;
    CHECK_THROWS(codec.decode(dup), invalid_argument);

    mt19937 rng(5);
    FloatChromosome bad = codec.createFloat(rng);
    bad[0] = 2.0;
    CHECK_THROWS(codec.decode(bad), invalid_argument);
}

void testEncodeRejectsInvalidRoutes() {
    CVRPCodec codec(5, 2);

    CHECK_THROWS(codec.encodeInteger({{1, 2}, {3, 4}}),           
                 invalid_argument);
    CHECK_THROWS(codec.encodeInteger({{1, 2, 3}, {3, 4, 5}}),     
                 invalid_argument);
    CHECK_THROWS(codec.encodeInteger({{1, 2, 3}, {4, 6}}),        
                 invalid_argument);
    CHECK_THROWS(codec.encodeInteger({{0, 1, 2}, {3, 4, 5}}),     
                 invalid_argument);
    CHECK_THROWS(codec.encodeInteger({{1}, {2}, {3, 4, 5}}),      
                 invalid_argument);
    CHECK_THROWS(codec.encodeFloat({{1, 2}, {3, 4}}),
                 invalid_argument);
    CHECK_THROWS(codec.encodeBinary({{1, 2}, {3, 4}}),
                 invalid_argument);

    CHECK(codec.decode(codec.encodeInteger({{5, 1, 2, 3, 4}})) ==
          padded({{5, 1, 2, 3, 4}}, 2));
}

void testEncodeBinaryNeedsEnoughBits() {
    CVRPCodec tooSmall(7, 3, 3);
    CHECK_THROWS(tooSmall.encodeBinary({{1, 2, 3}, {4, 5}, {6, 7}}),
                 invalid_argument);
    CVRPCodec ok(7, 3, 4);
    Routes r = {{1, 2, 3}, {4, 5}, {6, 7}};
    CHECK(ok.decode(ok.encodeBinary(r)) == r);
}

void testCreatedChromosomesWorkThroughBasePointer() {
    mt19937 rng(31);
    CVRPCodec codec(10, 3);

    unique_ptr<Chromosome> pop[] = {
        codec.createInteger(rng).clone(),
        codec.createFloat(rng).clone(),
        codec.createBinary(rng).clone(),
    };

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
