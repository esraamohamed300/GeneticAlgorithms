#include "TestUtils.h"

#include "IntegerChromosome.h"
#include "Replacement.h"

#include <cmath>
#include <limits>
using namespace std;


unique_ptr<Chromosome> makeIndividual(int value) {
    return make_unique<IntegerChromosome>(vector<int>{value}, 0, 1000);
}

Population makePopulation(const vector<double>& fitness) {
    Population p;
    for (double f : fitness) {
        p.addIndividual(makeIndividual(static_cast<int>(f)));
    }
    return p;
}

int geneOf(const Population& p, int i) {
    return dynamic_cast<const IntegerChromosome&>(p.getIndividual(i))[0];
}

bool operator==(const Survivor& a, const Survivor& b) {
    return a.fromOffspring == b.fromOffspring && a.index == b.index;
}

void testGenerationalWithElitism() {
    vector<double> parents = {5, 9, 1, 7};
    vector<double> kids    = {6, 2, 8, 3};

    auto s = GenerationalReplacement(1).chooseSurvivors(parents, kids);

    CHECK(s.size() == 4);
    CHECK((s[0] == Survivor{false, 1}));
    CHECK((s[1] == Survivor{true, 2}));
    CHECK((s[2] == Survivor{true, 0}));
    CHECK((s[3] == Survivor{true, 3}));
}

void testGenerationalWithoutElitismTakesOffspring() {
    auto s = GenerationalReplacement(0).chooseSurvivors({5, 9, 1, 7}, {6, 2, 8, 3});
    CHECK(s.size() == 4);
    for (const Survivor& x : s) {
        CHECK(x.fromOffspring);
    }
    CHECK((s[0] == Survivor{true, 2}));
    CHECK((s[3] == Survivor{true, 1}));
}

void testGenerationalFewOffspringFilledByBestParents() {
    auto s = GenerationalReplacement(1).chooseSurvivors({5, 9, 1, 7}, {6});
    CHECK(s.size() == 4);
    CHECK((s[0] == Survivor{false, 1}));
    CHECK((s[1] == Survivor{true, 0}));
    CHECK((s[2] == Survivor{false, 3}));
    CHECK((s[3] == Survivor{false, 0}));
}

void testGenerationalSizeAlwaysConstant() {
    for (int elite : {0u, 1u, 3u, 4u, 10u}) {
        for (int kids : {0u, 1u, 4u, 9u}) {
            vector<double> p = {4, 8, 2, 6};
            vector<double> c(kids, 5.0);
            auto s = GenerationalReplacement(elite).chooseSurvivors(p, c);
            CHECK(s.size() == p.size());
            for (const Survivor& x : s) {
                CHECK(x.fromOffspring ? x.index < kids : x.index < p.size());
            }
        }
    }
}

void testGenerationalKeepsBestParentWhenElitist() {
    auto s = GenerationalReplacement(1).chooseSurvivors({100, 1, 2}, {0, 0, 0});
    bool bestKept = false;
    for (const Survivor& x : s) {
        bestKept |= (!x.fromOffspring && x.index == 0);
    }
    CHECK(bestKept);
}

void testSteadyStateReplacesWorstWhenBetter() {
    vector<double> parents = {5, 9, 1, 7};
    vector<double> kids    = {6, 2};

    auto s = SteadyStateReplacement().chooseSurvivors(parents, kids);

    CHECK(s.size() == 4);
    CHECK((s[0] == Survivor{false, 0}));
    CHECK((s[1] == Survivor{false, 1}));
    CHECK((s[2] == Survivor{true, 0}));
    CHECK((s[3] == Survivor{false, 3}));
}

void testSteadyStateUnconditionalMode() {
    auto s = SteadyStateReplacement(false).chooseSurvivors({5, 9, 1, 7}, {6, 2});
    CHECK((s[2] == Survivor{true, 0}));
    CHECK((s[0] == Survivor{true, 1}));
    CHECK((s[1] == Survivor{false, 1}));
    CHECK((s[3] == Survivor{false, 3}));
}

void testSteadyStateNoOffspringChangesNothing() {
    auto s = SteadyStateReplacement().chooseSurvivors({5, 9, 1}, {});
    CHECK(s.size() == 3);
    for (int i = 0; i < s.size(); ++i) {
        CHECK((s[i] == Survivor{false, i}));
    }
}

void testSteadyStateNeverReplacesMoreThanPopulation() {
    auto s = SteadyStateReplacement(false).chooseSurvivors({1, 2}, {5, 6, 7, 8, 9});
    CHECK(s.size() == 2);
    CHECK(s[0].fromOffspring && s[1].fromOffspring);
}

void testSteadyStateNeverGetsWorse() {
    vector<double> parents = {3, 8, 5, 1, 9, 2};
    vector<double> kids = {4, 0, 7};
    auto s = SteadyStateReplacement().chooseSurvivors(parents, kids);

    double before = 0, after = 0;
    for (double f : parents) before += f;
    for (const Survivor& x : s) {
        after += x.fromOffspring ? kids[x.index] : parents[x.index];
    }
    CHECK(after >= before);
}

void testReplaceKeepsSizeAndAlignment() {
    vector<double> fitness = {5, 9, 1, 7};
    Population population = makePopulation(fitness);

    vector<double> kidFitness = {6, 2, 8, 3};
    Population kids = makePopulation(kidFitness);

    GenerationalReplacement(1).replace(population, fitness, kids, kidFitness);

    CHECK(population.size() == 4);
    CHECK(fitness.size() == 4);
    for (int i = 0; i < population.size(); ++i) {
        CHECK(geneOf(population, i) == static_cast<int>(fitness[i]));
    }
    CHECK((fitness == vector<double>{9, 8, 6, 3}));

    CHECK(kids.size() == 4);
    dynamic_cast<IntegerChromosome&>(population.getIndividual(1))[0] = 777;
    CHECK(geneOf(kids, 2) == 8);
}

void testReplaceSteadyStateAtPopulationLevel() {
    vector<double> fitness = {5, 9, 1, 7};
    Population population = makePopulation(fitness);
    vector<double> kidFitness = {6, 2};
    Population kids = makePopulation(kidFitness);

    SteadyStateReplacement().replace(population, fitness, kids, kidFitness);

    CHECK(population.size() == 4);
    CHECK((fitness == vector<double>{5, 9, 6, 7}));
    for (int i = 0; i < population.size(); ++i) {
        CHECK(geneOf(population, i) == static_cast<int>(fitness[i]));
    }
}

void testReplaceRejectsMismatchedSizes() {
    vector<double> fitness = {1, 2, 3};
    Population population = makePopulation({1, 2, 3});
    Population kids = makePopulation({4});
    vector<double> kidFitness = {4};

    vector<double> shortFitness = {1, 2};
    CHECK_THROWS(GenerationalReplacement().replace(population, shortFitness, kids, kidFitness),
                 invalid_argument);
    CHECK_THROWS(GenerationalReplacement().replace(population, fitness, kids, {4, 5}),
                 invalid_argument);
}

void testRejectsEmptyPopulationAndBadFitness() {
    CHECK_THROWS(GenerationalReplacement().chooseSurvivors({}, {1}),
                 invalid_argument);
    CHECK_THROWS(SteadyStateReplacement().chooseSurvivors({}, {1}),
                 invalid_argument);
    CHECK_THROWS(GenerationalReplacement().chooseSurvivors(
                     {1, numeric_limits<double>::quiet_NaN()}, {1}),
                 invalid_argument);
    CHECK_THROWS(SteadyStateReplacement().chooseSurvivors(
                     {1, 2}, {numeric_limits<double>::infinity()}),
                 invalid_argument);
}

void testNamesAndDefaults() {
    CHECK(GenerationalReplacement().name() == "Generational");
    CHECK(GenerationalReplacement().elitism() == 1);
    CHECK(SteadyStateReplacement().name() == "SteadyState");
    CHECK(SteadyStateReplacement().onlyIfBetter());
}

int main() {
    RUN_TEST(testGenerationalWithElitism);
    RUN_TEST(testGenerationalWithoutElitismTakesOffspring);
    RUN_TEST(testGenerationalFewOffspringFilledByBestParents);
    RUN_TEST(testGenerationalSizeAlwaysConstant);
    RUN_TEST(testGenerationalKeepsBestParentWhenElitist);
    RUN_TEST(testSteadyStateReplacesWorstWhenBetter);
    RUN_TEST(testSteadyStateUnconditionalMode);
    RUN_TEST(testSteadyStateNoOffspringChangesNothing);
    RUN_TEST(testSteadyStateNeverReplacesMoreThanPopulation);
    RUN_TEST(testSteadyStateNeverGetsWorse);
    RUN_TEST(testReplaceKeepsSizeAndAlignment);
    RUN_TEST(testReplaceSteadyStateAtPopulationLevel);
    RUN_TEST(testReplaceRejectsMismatchedSizes);
    RUN_TEST(testRejectsEmptyPopulationAndBadFitness);
    RUN_TEST(testNamesAndDefaults);
    return finishTests();
}