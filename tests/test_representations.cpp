#include "TestUtils.h"

#include "BinaryChromosome.h"
#include "FloatChromosome.h"
#include "IntegerChromosome.h"

#include <cmath>
#include <limits>
#include <memory>
#include <random>
using namespace std;


void testBinaryRandomIsValid() {
    mt19937 rng(1);
    for (int length : {0u, 1u, 7u, 64u, 1000u}) {
        BinaryChromosome c = BinaryChromosome::random(length, rng);
        CHECK(c.size() == length);
        CHECK(c.isValid());
        for (int i = 0; i < c.size(); ++i) {
            CHECK(c[i] == 0 || c[i] == 1);
        }
    }
}

void testBinaryRandomHasBothValues() {
    mt19937 rng(2);
    BinaryChromosome c = BinaryChromosome::random(500, rng);
    int ones = 0;
    for (int i = 0; i < c.size(); ++i) {
        ones += c[i];
    }
    CHECK(ones > 150 && ones < 350);
}

void testBinaryRejectsInvalidGenes() {
    CHECK_THROWS(BinaryChromosome({0, 1, 2}), invalid_argument);

    BinaryChromosome c({0, 1, 1, 0});
    c[0] = 5;
    CHECK(!c.isValid());
}

void testBinaryUnsignedRoundTrip() {
    BinaryChromosome c({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    c.encodeUnsigned(0, 5, 19);
    c.encodeUnsigned(5, 7, 100);
    CHECK(c.toString() == "100111100100");
    CHECK(c.decodeUnsigned(0, 5) == 19u);
    CHECK(c.decodeUnsigned(5, 7) == 100u);
    CHECK(c.decodeUnsigned(0, 12) == 0b100111100100u);

    CHECK_THROWS(c.encodeUnsigned(0, 3, 8), invalid_argument);
    CHECK_THROWS(c.encodeUnsigned(10, 5, 1), out_of_range);
    CHECK_THROWS(c.decodeUnsigned(10, 5), out_of_range);
    CHECK_THROWS(c.decodeUnsigned(0, 0), invalid_argument);
    CHECK_THROWS(c.decodeUnsigned(0, 33), invalid_argument);
}

void testBinaryFlipAndClone() {
    BinaryChromosome c({1, 0, 1, 1});
    c.flip(1);
    CHECK(c.toString() == "1111");
    c.flip(2);
    CHECK(c.toString() == "1101");
    c.flip(1);
    c.flip(2);
    CHECK(c.toString() == "1011");
    CHECK_THROWS(c.flip(4), out_of_range);

    unique_ptr<Chromosome> copy = c.clone();
    auto* typed = dynamic_cast<BinaryChromosome*>(copy.get());
    CHECK(typed != nullptr);
    CHECK(typed->toString() == "1011");

    typed->flip(0);
    CHECK(c.toString() == "1011");
    CHECK(typed->toString() == "0011");
}

void testBinarySeedReproducible() {
    mt19937 a(42), b(42);
    CHECK(BinaryChromosome::random(100, a).genes() ==
          BinaryChromosome::random(100, b).genes());
}

void testIntegerRandomInRange() {
    mt19937 rng(3);
    IntegerChromosome c = IntegerChromosome::random(200, -5, 5, rng);
    CHECK(c.size() == 200);
    CHECK(c.isValid());
    CHECK(!c.isPermutation());
    CHECK(c.lowerBound() == -5 && c.upperBound() == 5);

    bool sawLow = false, sawHigh = false;
    for (int g : c.genes()) {
        CHECK(g >= -5 && g <= 5);
        sawLow |= (g == -5);
        sawHigh |= (g == 5);
    }
    CHECK(sawLow && sawHigh);
}

void testIntegerRandomPermutation() {
    mt19937 rng(4);
    for (int n : {1, 2, 10, 101}) {
        IntegerChromosome c = IntegerChromosome::randomPermutation(1, n, rng);
        CHECK(static_cast<int>(c.size()) == n);
        CHECK(c.isPermutation());
        CHECK(c.isValid());
    }

    IntegerChromosome neg = IntegerChromosome::randomPermutation(-2, 2, rng);
    CHECK(neg.size() == 5);
    CHECK(neg.isValid());

    IntegerChromosome big = IntegerChromosome::randomPermutation(1, 50, rng);
    bool identity = true;
    for (int i = 0; i < 50; ++i) {
        identity &= (big[static_cast<int>(i)] == i + 1);
    }
    CHECK(!identity);
}

void testIntegerRejectsInvalid() {
    mt19937 rng(1);
    CHECK_THROWS(IntegerChromosome({1, 2, 9}, 1, 5), invalid_argument);
    CHECK_THROWS(IntegerChromosome({1, 2}, 5, 1), invalid_argument);
    CHECK_THROWS(IntegerChromosome({1, 1, 3}, 1, 3, true),
                 invalid_argument);
    CHECK_THROWS(IntegerChromosome({1, 2}, 1, 3, true),
                 invalid_argument);
    CHECK_THROWS(IntegerChromosome::random(5, 3, 1, rng),
                 invalid_argument);
    CHECK_THROWS(IntegerChromosome::randomPermutation(3, 1, rng),
                 invalid_argument);
}

void testIntegerIsValidDetectsTampering() {
    IntegerChromosome c({3, 1, 2}, 1, 3, true);
    CHECK(c.isValid());

    c[0] = 1;
    CHECK(!c.isValid());

    c[0] = 99;
    CHECK(!c.isValid());
}

void testIntegerClone() {
    IntegerChromosome c({3, 1, 2}, 1, 3, true);
    unique_ptr<Chromosome> copy = c.clone();
    auto* typed = dynamic_cast<IntegerChromosome*>(copy.get());
    CHECK(typed != nullptr);
    CHECK(typed->genes() == c.genes());
    CHECK(typed->isPermutation());
    CHECK(typed->lowerBound() == 1 && typed->upperBound() == 3);

    (*typed)[0] = 2;
    (*typed)[2] = 3;
    CHECK(c.toString() == "3 1 2");
    CHECK(typed->toString() == "2 1 3");
}

void testIntegerSeedReproducible() {
    mt19937 a(7), b(7);
    CHECK(IntegerChromosome::randomPermutation(1, 30, a).genes() ==
          IntegerChromosome::randomPermutation(1, 30, b).genes());
}

void testFloatRandomInRange() {
    mt19937 rng(5);
    FloatChromosome c = FloatChromosome::random(300, -2.5, 4.0, rng);
    CHECK(c.size() == 300);
    CHECK(c.isValid());
    CHECK(c.lowerBound() == -2.5 && c.upperBound() == 4.0);
    for (double g : c.genes()) {
        CHECK(g >= -2.5 && g <= 4.0);
    }
}

void testFloatDegenerateRange() {
    mt19937 rng(6);
    FloatChromosome c = FloatChromosome::random(5, 1.5, 1.5, rng);
    CHECK(c.isValid());
    for (double g : c.genes()) {
        CHECK(g == 1.5);
    }
}

void testFloatRejectsInvalid() {
    CHECK_THROWS(FloatChromosome({0.5, 2.0}, 0.0, 1.0), invalid_argument);
    CHECK_THROWS(FloatChromosome({0.5}, 1.0, 0.0), invalid_argument);
    CHECK_THROWS(
        FloatChromosome({numeric_limits<double>::quiet_NaN()}, 0.0, 1.0),
        invalid_argument);
    CHECK_THROWS(
        FloatChromosome({0.5}, 0.0, numeric_limits<double>::infinity()),
        invalid_argument);
}

void testFloatClampRepairs() {
    FloatChromosome c({0.2, 0.8, 0.5}, 0.0, 1.0);
    c[0] = -3.0;
    c[1] = 7.0;
    c[2] = numeric_limits<double>::quiet_NaN();
    CHECK(!c.isValid());

    c.clamp();
    CHECK(c.isValid());
    CHECK(c[0] == 0.0);
    CHECK(c[1] == 1.0);
    CHECK(c[2] == 0.0);
}

void testFloatCloneAndSeed() {
    FloatChromosome c({0.1, 0.9}, 0.0, 1.0);
    unique_ptr<Chromosome> copy = c.clone();
    auto* typed = dynamic_cast<FloatChromosome*>(copy.get());
    CHECK(typed != nullptr);
    CHECK(typed->genes() == c.genes());
    (*typed)[0] = 0.7;
    CHECK(c[0] == 0.1);

    mt19937 a(11), b(11);
    CHECK(FloatChromosome::random(20, 0.0, 1.0, a).genes() ==
          FloatChromosome::random(20, 0.0, 1.0, b).genes());
}

void testAllRepresentationsShareChromosomeInterface() {
    mt19937 rng(9);
    BinaryChromosome b = BinaryChromosome::random(8, rng);
    IntegerChromosome i = IntegerChromosome::randomPermutation(1, 8, rng);
    FloatChromosome f = FloatChromosome::random(8, 0.0, 1.0, rng);

    unique_ptr<Chromosome> pool[] = {b.clone(), i.clone(), f.clone()};
    for (auto& p : pool) {
        CHECK(p != nullptr);
        CHECK(p->clone() != nullptr);
    }
    CHECK(dynamic_cast<BinaryChromosome*>(pool[0].get()) != nullptr);
    CHECK(dynamic_cast<IntegerChromosome*>(pool[1].get()) != nullptr);
    CHECK(dynamic_cast<FloatChromosome*>(pool[2].get()) != nullptr);
}

int main() {
    RUN_TEST(testBinaryRandomIsValid);
    RUN_TEST(testBinaryRandomHasBothValues);
    RUN_TEST(testBinaryRejectsInvalidGenes);
    RUN_TEST(testBinaryUnsignedRoundTrip);
    RUN_TEST(testBinaryFlipAndClone);
    RUN_TEST(testBinarySeedReproducible);

    RUN_TEST(testIntegerRandomInRange);
    RUN_TEST(testIntegerRandomPermutation);
    RUN_TEST(testIntegerRejectsInvalid);
    RUN_TEST(testIntegerIsValidDetectsTampering);
    RUN_TEST(testIntegerClone);
    RUN_TEST(testIntegerSeedReproducible);

    RUN_TEST(testFloatRandomInRange);
    RUN_TEST(testFloatDegenerateRange);
    RUN_TEST(testFloatRejectsInvalid);
    RUN_TEST(testFloatClampRepairs);
    RUN_TEST(testFloatCloneAndSeed);

    RUN_TEST(testAllRepresentationsShareChromosomeInterface);

    return finishTests();
}