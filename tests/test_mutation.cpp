#include "TestUtils.h"

#include "BinaryChromosome.h"
#include "FloatChromosome.h"
#include "IntegerChromosome.h"
#include "Mutation.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace std;

void testBitFlipRateZero() {
    mt19937 rng(1);
    BitFlipMutation mut(0.0);

    BinaryChromosome b({1, 0, 1, 1, 0});
    auto mutated = mut.mutate(b, rng);
    auto* typed = dynamic_cast<BinaryChromosome*>(mutated.get());

    CHECK(typed != nullptr);
    CHECK(typed->toString() == "10110");
    CHECK(b.toString() == "10110");
}

void testBitFlipRateOneFlipsAll() {
    mt19937 rng(2);
    BitFlipMutation mut(1.0);

    BinaryChromosome b({1, 0, 1, 1, 0});
    auto mutated = mut.mutate(b, rng);
    auto* typed = dynamic_cast<BinaryChromosome*>(mutated.get());

    CHECK(typed != nullptr);
    CHECK(typed->toString() == "01001");
    CHECK(typed->isValid());
}

void testBitFlipIntermediateRate() {
    mt19937 rng(3);
    BitFlipMutation mut(0.1);

    const int len = 2000;
    BinaryChromosome b = BinaryChromosome::random(len, rng);
    auto mutated = mut.mutate(b, rng);
    auto* typed = static_cast<BinaryChromosome*>(mutated.get());

    CHECK(typed->isValid());
    int diffs = 0;
    for (int i = 0; i < len; ++i) {
        if ((*typed)[i] != b[i]) {
            ++diffs;
        }
    }
    CHECK(diffs > 120 && diffs < 280);
}

void testBitFlipRejectsNonBinary() {
    mt19937 rng(4);
    BitFlipMutation mut(0.05);

    IntegerChromosome ic({1, 2, 3}, 1, 3, true);
    CHECK_THROWS(mut.mutate(ic, rng), invalid_argument);

    FloatChromosome fc({0.5, 0.2}, 0.0, 1.0);
    CHECK_THROWS(mut.mutate(fc, rng), invalid_argument);
}

void testBitFlipEdgeCases() {
    mt19937 rng(5);
    BitFlipMutation mut(0.5);

    BinaryChromosome empty(vector<uint8_t>{});
    auto mEmpty = mut.mutate(empty, rng);
    CHECK(static_cast<BinaryChromosome*>(mEmpty.get())->size() == 0);

    BinaryChromosome single({1});
    BitFlipMutation mutAlways(1.0);
    auto mSingle = mutAlways.mutate(single, rng);
    CHECK(static_cast<BinaryChromosome*>(mSingle.get())->toString() == "0");

    CHECK_THROWS(BitFlipMutation(-0.01), invalid_argument);
    CHECK_THROWS(BitFlipMutation(1.05), invalid_argument);
}

void testSwapPermutationPreservation() {
    mt19937 rng(42);
    SwapMutation mut(1.0);

    IntegerChromosome perm({1, 2, 3, 4, 5, 6, 7, 8}, 1, 8, true);

    for (int t = 0; t < 50; ++t) {
        auto mutated = mut.mutate(perm, rng);
        auto* typed = dynamic_cast<IntegerChromosome*>(mutated.get());

        CHECK(typed != nullptr);
        CHECK(typed->size() == 8);
        CHECK(typed->isPermutation());
        CHECK(typed->isValid());

        int diffs = 0;
        for (int i = 0; i < 8; ++i) {
            if ((*typed)[i] != perm[i]) {
                ++diffs;
            }
        }
        CHECK(diffs == 2);
    }
}

void testSwapRateZeroLeavesIntact() {
    mt19937 rng(7);
    SwapMutation mut(0.0);

    IntegerChromosome perm({1, 2, 3, 4}, 1, 4, true);
    auto mutated = mut.mutate(perm, rng);
    auto* typed = static_cast<IntegerChromosome*>(mutated.get());

    CHECK(typed->genes() == perm.genes());
}

void testSwapWorksOnFloatAndBinary() {
    mt19937 rng(8);
    SwapMutation mut(1.0);

    FloatChromosome fc({0.1, 0.5, 0.9}, 0.0, 1.0);
    auto mfc = mut.mutate(fc, rng);
    auto* typedFc = dynamic_cast<FloatChromosome*>(mfc.get());
    CHECK(typedFc != nullptr);
    CHECK(typedFc->isValid());
    CHECK(typedFc->size() == 3);

    BinaryChromosome bc({1, 0, 0, 0});
    auto mbc = mut.mutate(bc, rng);
    auto* typedBc = dynamic_cast<BinaryChromosome*>(mbc.get());
    CHECK(typedBc != nullptr);
    CHECK(typedBc->isValid());
    int sum = 0;
    for (int i = 0; i < 4; ++i) sum += (*typedBc)[i];
    CHECK(sum == 1);
}

void testSwapShortChromosomes() {
    mt19937 rng(9);
    SwapMutation mut(1.0);

    IntegerChromosome len0(vector<int>{}, 0, 5, false);
    auto m0 = mut.mutate(len0, rng);
    CHECK(static_cast<IntegerChromosome*>(m0.get())->size() == 0);

    IntegerChromosome len1({5}, 5, 5, true);
    auto m1 = mut.mutate(len1, rng);
    CHECK(static_cast<IntegerChromosome*>(m1.get())->genes() == len1.genes());
}

void testInversionPermutationPreservation() {
    mt19937 rng(101);
    InversionMutation mut(1.0);

    IntegerChromosome perm({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 1, 10, true);

    for (int t = 0; t < 50; ++t) {
        auto mutated = mut.mutate(perm, rng);
        auto* typed = dynamic_cast<IntegerChromosome*>(mutated.get());

        CHECK(typed != nullptr);
        CHECK(typed->size() == 10);
        CHECK(typed->isPermutation());
        CHECK(typed->isValid());

        vector<int> sorted = typed->genes();
        sort(sorted.begin(), sorted.end());
        for (int v = 1; v <= 10; ++v) {
            CHECK(sorted[v - 1] == v);
        }
    }
}

void testInversionReversesSlice() {
    mt19937 rng(55);
    InversionMutation mut(1.0);

    IntegerChromosome perm({1, 2, 3, 4, 5}, 1, 5, true);
    mut.mutateInPlace(perm, rng);
    CHECK(perm.isValid());
    CHECK(perm.isPermutation());
}

void testInversionWorksOnFloat() {
    mt19937 rng(66);
    InversionMutation mut(1.0);

    FloatChromosome fc({0.1, 0.2, 0.3, 0.4}, 0.0, 1.0);
    auto mfc = mut.mutate(fc, rng);
    auto* typed = dynamic_cast<FloatChromosome*>(mfc.get());

    CHECK(typed != nullptr);
    CHECK(typed->isValid());
    CHECK(typed->lowerBound() == 0.0 && typed->upperBound() == 1.0);
}

void testInversionShortChromosomes() {
    mt19937 rng(77);
    InversionMutation mut(1.0);

    IntegerChromosome len1({42}, 42, 42, true);
    auto m1 = mut.mutate(len1, rng);
    CHECK(static_cast<IntegerChromosome*>(m1.get())->genes() == len1.genes());
    CHECK(static_cast<IntegerChromosome*>(m1.get())->isValid());
}

int main() {
    RUN_TEST(testBitFlipRateZero);
    RUN_TEST(testBitFlipRateOneFlipsAll);
    RUN_TEST(testBitFlipIntermediateRate);
    RUN_TEST(testBitFlipRejectsNonBinary);
    RUN_TEST(testBitFlipEdgeCases);

    RUN_TEST(testSwapPermutationPreservation);
    RUN_TEST(testSwapRateZeroLeavesIntact);
    RUN_TEST(testSwapWorksOnFloatAndBinary);
    RUN_TEST(testSwapShortChromosomes);

    RUN_TEST(testInversionPermutationPreservation);
    RUN_TEST(testInversionReversesSlice);
    RUN_TEST(testInversionWorksOnFloat);
    RUN_TEST(testInversionShortChromosomes);

    return finishTests();
}