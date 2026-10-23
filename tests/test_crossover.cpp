#include "TestUtils.h"

#include "BinaryChromosome.h"
#include "Crossover.h"
#include "FloatChromosome.h"
#include "IntegerChromosome.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace std;


void testMultiPointBinary() {
    mt19937 rng(42);
    MultiPointCrossover cross2(2, 1.0);  

    BinaryChromosome p1({0, 0, 0, 0, 0, 0, 0, 0});
    BinaryChromosome p2({1, 1, 1, 1, 1, 1, 1, 1});

    auto [c1, c2] = cross2.cross(p1, p2, rng);

    CHECK(c1 != nullptr && c2 != nullptr);
    CHECK(c1->clone() != nullptr);
    CHECK(dynamic_cast<BinaryChromosome*>(c1.get()) != nullptr);
    CHECK(dynamic_cast<BinaryChromosome*>(c2.get()) != nullptr);

    auto* b1 = static_cast<BinaryChromosome*>(c1.get());
    auto* b2 = static_cast<BinaryChromosome*>(c2.get());

    CHECK(b1->size() == 8);
    CHECK(b2->size() == 8);
    CHECK(b1->isValid());
    CHECK(b2->isValid());

    CHECK(p1.toString() == "00000000");
    CHECK(p2.toString() == "11111111");

  
    for (int i = 0; i < 8; ++i) {
        CHECK((*b1)[i] + (*b2)[i] == 1);
    }
}

void testMultiPointFloat() {
    mt19937 rng(123);
    MultiPointCrossover cross3(3, 1.0);

    FloatChromosome p1({0.1, 0.1, 0.1, 0.1, 0.1, 0.1}, 0.0, 1.0);
    FloatChromosome p2({0.9, 0.9, 0.9, 0.9, 0.9, 0.9}, 0.0, 1.0);

    auto [c1, c2] = cross3.cross(p1, p2, rng);
    auto* f1 = dynamic_cast<FloatChromosome*>(c1.get());
    auto* f2 = dynamic_cast<FloatChromosome*>(c2.get());

    CHECK(f1 != nullptr && f2 != nullptr);
    CHECK(f1->size() == 6 && f2->size() == 6);
    CHECK(f1->isValid() && f2->isValid());
    CHECK(f1->lowerBound() == 0.0 && f1->upperBound() == 1.0);

    for (int i = 0; i < 6; ++i) {
        CHECK((*f1)[i] == 0.1 || (*f1)[i] == 0.9);
        CHECK((*f2)[i] == 0.1 || (*f2)[i] == 0.9);
    }
}

void testMultiPointFreeInteger() {
    mt19937 rng(99);
    MultiPointCrossover cross1(1, 1.0);  

    IntegerChromosome p1({10, 10, 10, 10}, 0, 50, false);
    IntegerChromosome p2({20, 20, 20, 20}, 0, 50, false);

    auto [c1, c2] = cross1.cross(p1, p2, rng);
    auto* i1 = dynamic_cast<IntegerChromosome*>(c1.get());
    auto* i2 = dynamic_cast<IntegerChromosome*>(c2.get());

    CHECK(i1 != nullptr && i2 != nullptr);
    CHECK(i1->size() == 4 && i2->size() == 4);
    CHECK(!i1->isPermutation() && !i2->isPermutation());
    CHECK(i1->isValid() && i2->isValid());
}

void testMultiPointRateZeroClonesParents() {
    mt19937 rng(7);
    MultiPointCrossover crossZero(2, 0.0);  

    BinaryChromosome p1({0, 1, 0, 1});
    BinaryChromosome p2({1, 0, 1, 0});

    auto [c1, c2] = crossZero.cross(p1, p2, rng);
    auto* b1 = static_cast<BinaryChromosome*>(c1.get());
    auto* b2 = static_cast<BinaryChromosome*>(c2.get());

    CHECK(b1->toString() == "0101");
    CHECK(b2->toString() == "1010");
}

void testMultiPointRejectsPermutationChromosomes() {
    mt19937 rng(1);
    MultiPointCrossover cross(2, 1.0);

    IntegerChromosome perm1 = IntegerChromosome::randomPermutation(1, 6, rng);
    IntegerChromosome perm2 = IntegerChromosome::randomPermutation(1, 6, rng);

 
    CHECK_THROWS(cross.cross(perm1, perm2, rng), invalid_argument);
}

void testMultiPointEdgeCases() {
    mt19937 rng(5);
    MultiPointCrossover cross(5, 1.0);  

   
    BinaryChromosome empty1(vector<uint8_t>{});
    BinaryChromosome empty2(vector<uint8_t>{});
    auto [e1, e2] = cross.cross(empty1, empty2, rng);
    CHECK(e1->clone() != nullptr && static_cast<BinaryChromosome*>(e1.get())->size() == 0);

   
    BinaryChromosome single1({1}), single2({0});
    auto [s1, s2] = cross.cross(single1, single2, rng);
    CHECK(static_cast<BinaryChromosome*>(s1.get())->size() == 1);

   
    BinaryChromosome bLen2({0, 1}), bLen3({0, 1, 0});
    CHECK_THROWS(cross.cross(bLen2, bLen3, rng), invalid_argument);

   
    CHECK_THROWS(MultiPointCrossover(0, 0.9), invalid_argument);
    CHECK_THROWS(MultiPointCrossover(2, -0.1), invalid_argument);
    CHECK_THROWS(MultiPointCrossover(2, 1.1), invalid_argument);
}



void testOXPreservesPermutationInvariants() {
    mt19937 rng(42);
    OrderCrossover ox(1.0);


    IntegerChromosome p1({1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 9, true);
    IntegerChromosome p2({9, 8, 7, 6, 5, 4, 3, 2, 1}, 1, 9, true);

    for (int trial = 0; trial < 100; ++trial) {
        auto [c1, c2] = ox.cross(p1, p2, rng);

        auto* child1 = dynamic_cast<IntegerChromosome*>(c1.get());
        auto* child2 = dynamic_cast<IntegerChromosome*>(c2.get());

        CHECK(child1 != nullptr && child2 != nullptr);
        CHECK(child1->size() == 9 && child2->size() == 9);
        CHECK(child1->isPermutation() && child2->isPermutation());
        CHECK(child1->isValid() && child2->isValid());

       
        vector<int> g1 = child1->genes();
        vector<int> g2 = child2->genes();
        sort(g1.begin(), g1.end());
        sort(g2.begin(), g2.end());
        for (int val = 1; val <= 9; ++val) {
            CHECK(g1[val - 1] == val);
            CHECK(g2[val - 1] == val);
        }
    }
}

void testOXWithNegativeBounds() {
    mt19937 rng(77);
    OrderCrossover ox(1.0);

    IntegerChromosome p1({-2, 0, 1, -1, 2}, -2, 2, true);
    IntegerChromosome p2({2, 1, 0, -1, -2}, -2, 2, true);

    auto [c1, c2] = ox.cross(p1, p2, rng);
    auto* child1 = dynamic_cast<IntegerChromosome*>(c1.get());
    auto* child2 = dynamic_cast<IntegerChromosome*>(c2.get());

    CHECK(child1->isValid() && child2->isValid());
    CHECK(child1->isPermutation() && child2->isPermutation());
    CHECK(child1->lowerBound() == -2 && child1->upperBound() == 2);
}

void testOXRateZeroClonesParents() {
    mt19937 rng(88);
    OrderCrossover ox(0.0);  

    IntegerChromosome p1({1, 2, 3, 4}, 1, 4, true);
    IntegerChromosome p2({4, 3, 2, 1}, 1, 4, true);

    auto [c1, c2] = ox.cross(p1, p2, rng);
    auto* child1 = static_cast<IntegerChromosome*>(c1.get());
    auto* child2 = static_cast<IntegerChromosome*>(c2.get());

    CHECK(child1->genes() == p1.genes());
    CHECK(child2->genes() == p2.genes());
}

void testOXRejectsNonPermutations() {
    mt19937 rng(10);
    OrderCrossover ox(1.0);

   
    IntegerChromosome free1({1, 2, 2, 4}, 1, 4, false);
    IntegerChromosome free2({4, 3, 2, 1}, 1, 4, false);
    CHECK_THROWS(ox.cross(free1, free2, rng), invalid_argument);

    
    IntegerChromosome permA({1, 2, 3}, 1, 3, true);
    IntegerChromosome permB({2, 3, 4}, 2, 4, true);
    CHECK_THROWS(ox.cross(permA, permB, rng), invalid_argument);

   
    BinaryChromosome bin({0, 1, 0});
    CHECK_THROWS(ox.cross(bin, permA, rng), invalid_argument);
}

void testOXShortPermutations() {
    mt19937 rng(15);
    OrderCrossover ox(1.0);

   
    IntegerChromosome pLen1({5}, 5, 5, true);
    auto [c1, c2] = ox.cross(pLen1, pLen1, rng);
    CHECK(static_cast<IntegerChromosome*>(c1.get())->size() == 1);
    CHECK(static_cast<IntegerChromosome*>(c1.get())->isValid());

  
    IntegerChromosome pA({1, 2}, 1, 2, true);
    IntegerChromosome pB({2, 1}, 1, 2, true);
    auto [c2A, c2B] = ox.cross(pA, pB, rng);
    CHECK(static_cast<IntegerChromosome*>(c2A.get())->isValid());
    CHECK(static_cast<IntegerChromosome*>(c2B.get())->isValid());
}

int main() {
    RUN_TEST(testMultiPointBinary);
    RUN_TEST(testMultiPointFloat);
    RUN_TEST(testMultiPointFreeInteger);
    RUN_TEST(testMultiPointRateZeroClonesParents);
    RUN_TEST(testMultiPointRejectsPermutationChromosomes);
    RUN_TEST(testMultiPointEdgeCases);

    RUN_TEST(testOXPreservesPermutationInvariants);
    RUN_TEST(testOXWithNegativeBounds);
    RUN_TEST(testOXRateZeroClonesParents);
    RUN_TEST(testOXRejectsNonPermutations);
    RUN_TEST(testOXShortPermutations);

    return finishTests();
}
