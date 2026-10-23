// Unit tests for TournamentSelection and RouletteWheelSelection.

#include "TestUtils.h"

#include "Selection.h"

#include <limits>
#include <random>

namespace {

// How often each index is picked over `draws` single selections.
std::vector<int> countPicks(const SelectionMethod& method,
                            const std::vector<double>& fitness, int draws,
                            unsigned seed) {
    std::mt19937 rng(seed);
    std::vector<int> counts(fitness.size(), 0);
    for (int i = 0; i < draws; ++i) {
        ++counts[method.select(fitness, rng)];
    }
    return counts;
}

}  // namespace

// ------------------------------------------------------------ Tournament ----

void testTournamentFavoursFitter() {
    // P(best) = 1 - 0.9^3 ~ 27%, P(worst) = 0.1^3 = 0.1% for size 3 of 10.
    std::vector<double> fitness = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const int draws = 20000;
    auto counts = countPicks(TournamentSelection(3), fitness, draws, 1);

    CHECK(counts[9] > 0.22 * draws && counts[9] < 0.32 * draws);
    CHECK(counts[0] < 0.01 * draws);
    CHECK(counts[9] > counts[5] && counts[5] > counts[1]);  // monotone pressure
}

void testTournamentSizeControlsPressure() {
    std::vector<double> fitness = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const int draws = 5000;

    auto big = countPicks(TournamentSelection(50), fitness, draws, 2);
    CHECK(big[9] > 0.95 * draws);  // almost always the best

    // Size 1 = no pressure: every individual picked about 10% of the time.
    auto one = countPicks(TournamentSelection(1), fitness, draws, 3);
    for (int c : one) {
        CHECK(c > 0.07 * draws && c < 0.13 * draws);
    }
}

void testTournamentWorksWithNegativeFitness() {
    std::vector<double> fitness = {-5.0, -1.0, -3.0};
    auto counts = countPicks(TournamentSelection(30), fitness, 1000, 4);
    CHECK(counts[1] > 950);  // -1 is the best value
}

void testTournamentEdgeCases() {
    std::mt19937 rng(5);
    TournamentSelection t;
    CHECK(t.tournamentSize() == 3);  // documented default
    CHECK(t.select({42.0}, rng) == 0);  // one individual

    CHECK_THROWS(TournamentSelection(0), std::invalid_argument);
    CHECK_THROWS(t.select({}, rng), std::invalid_argument);
    CHECK_THROWS(t.select({1.0, std::numeric_limits<double>::quiet_NaN()}, rng),
                 std::invalid_argument);
}

// ----------------------------------------------------------- Roulette ------

void testRouletteProportionalToFitness() {
    const int draws = 20000;
    auto counts = countPicks(RouletteWheelSelection(), {1.0, 3.0}, draws, 6);
    CHECK(counts[1] > 0.72 * draws && counts[1] < 0.78 * draws);  // 75%

    auto four = countPicks(RouletteWheelSelection(), {1, 2, 3, 4}, draws, 7);
    CHECK(four[3] > 0.36 * draws && four[3] < 0.44 * draws);  // 40%
    CHECK(four[0] > 0.07 * draws && four[0] < 0.13 * draws);  // 10%
}

void testRouletteNeverPicksZeroFitness() {
    auto counts = countPicks(RouletteWheelSelection(), {0, 5, 0, 5, 0}, 5000, 8);
    CHECK(counts[0] == 0 && counts[2] == 0 && counts[4] == 0);
    CHECK(counts[1] > 2000 && counts[3] > 2000);
}

void testRouletteShiftsNegativeFitness() {
    // {-2, 0, 2} is shifted to weights {0, 2, 4}: index 0 never, index 2 ~ 2/3.
    const int draws = 12000;
    auto counts = countPicks(RouletteWheelSelection(), {-2.0, 0.0, 2.0}, draws, 9);
    CHECK(counts[0] == 0);
    CHECK(counts[2] > 0.62 * draws && counts[2] < 0.71 * draws);
}

void testRouletteAllEqualFallsBackToUniform() {
    // All zero (and all equal negative) -> no valid weights -> uniform.
    for (std::vector<double> fitness :
         {std::vector<double>{0, 0, 0, 0}, std::vector<double>{-3, -3, -3, -3}}) {
        // equal negatives shift to all-zero weights as well
        auto counts = countPicks(RouletteWheelSelection(), fitness, 4000, 10);
        for (int c : counts) {
            CHECK(c > 700);  // each of 4 gets ~25%
        }
    }
}

void testRouletteEdgeCases() {
    std::mt19937 rng(11);
    RouletteWheelSelection r;
    CHECK(r.select({7.0}, rng) == 0);
    CHECK_THROWS(r.select({}, rng), std::invalid_argument);
    CHECK_THROWS(r.select({1.0, std::numeric_limits<double>::infinity()}, rng),
                 std::invalid_argument);
}

// ----------------------------------------------------------- selectMany ----

void testSelectManyReturnsRequestedCountInRange() {
    std::vector<double> fitness = {3, 1, 4, 1, 5, 9, 2, 6};
    std::mt19937 rng(12);

    TournamentSelection t(2);
    RouletteWheelSelection r;
    for (const SelectionMethod* m :
         std::vector<const SelectionMethod*>{&t, &r}) {
        auto picked = m->selectMany(fitness, 100, rng);
        CHECK(picked.size() == 100);
        for (std::size_t idx : picked) {
            CHECK(idx < fitness.size());
        }
        CHECK(m->selectMany(fitness, 0, rng).empty());
        CHECK_THROWS(m->selectMany({}, 3, rng), std::invalid_argument);
    }
}

void testSelectManyMatchesDistribution() {
    std::mt19937 rng(13);
    auto picked = RouletteWheelSelection().selectMany({1.0, 3.0}, 20000, rng);
    int ones = 0;
    for (std::size_t idx : picked) {
        ones += (idx == 1);
    }
    CHECK(ones > 0.72 * 20000 && ones < 0.78 * 20000);
}

void testSameSeedSameSelection() {
    std::vector<double> fitness = {1, 2, 3, 4, 5};
    std::mt19937 a(99), b(99);
    CHECK(TournamentSelection(3).selectMany(fitness, 50, a) ==
          TournamentSelection(3).selectMany(fitness, 50, b));
    std::mt19937 c(99), d(99);
    CHECK(RouletteWheelSelection().selectMany(fitness, 50, c) ==
          RouletteWheelSelection().selectMany(fitness, 50, d));
}

void testNames() {
    CHECK(TournamentSelection().name() == "Tournament");
    CHECK(RouletteWheelSelection().name() == "RouletteWheel");
}

int main() {
    RUN_TEST(testTournamentFavoursFitter);
    RUN_TEST(testTournamentSizeControlsPressure);
    RUN_TEST(testTournamentWorksWithNegativeFitness);
    RUN_TEST(testTournamentEdgeCases);
    RUN_TEST(testRouletteProportionalToFitness);
    RUN_TEST(testRouletteNeverPicksZeroFitness);
    RUN_TEST(testRouletteShiftsNegativeFitness);
    RUN_TEST(testRouletteAllEqualFallsBackToUniform);
    RUN_TEST(testRouletteEdgeCases);
    RUN_TEST(testSelectManyReturnsRequestedCountInRange);
    RUN_TEST(testSelectManyMatchesDistribution);
    RUN_TEST(testSameSeedSameSelection);
    RUN_TEST(testNames);
    return finishTests();
}
