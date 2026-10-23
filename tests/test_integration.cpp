#include "TestUtils.h"

#include "BinaryChromosome.h"
#include "CVRPCodec.h"
#include "CVRPFeasibility.h"
#include "Crossover.h"
#include "FloatChromosome.h"
#include "GAConfig.h"
#include "GAEngine.h"
#include "IntegerChromosome.h"
#include "Mutation.h"
#include "Population.h"
#include "Replacement.h"
#include "Selection.h"

#include <cmath>
#include <memory>
#include <random>
#include <vector>

using namespace std;

void testGAEngineCVRPEndToEnd() {
    mt19937 rng(42);

    // CVRP Setup: 5 customers, 2 vehicles -> Giant tour length L = 5 + 2 - 1 = 6
    const int numCustomers = 5;
    const int numVehicles = 2;
    CVRPCodec codec(numCustomers, numVehicles);

    // Demands: index 0 is depot (0), customers 1..5 demand 10 each
    vector<double> demands = {0.0, 10.0, 10.0, 10.0, 10.0, 10.0};
    const double capacity = 35.0;  // 35 > 30, so any 3-2 split is feasible
    CVRPFeasibilityChecker checker(codec, demands, capacity);

    // Simple distance-like fitness: higher is better.
    FitnessFunction fitnessFn = [&](const Chromosome& chrom) -> double {
        Routes routes;
        if (!codec.tryDecode(chrom, routes)) {
            return 0.0;
        }
        double score = 100.0;
        if (!checker.isFeasible(routes)) {
            score -= 50.0;
        }
        return score;
    };

    FeasibilityFunction feasFn = checker.asFunction();

    GAConfig config;
    config.populationSize = 20;
    config.generations = 15;
    config.crossoverRate = 0.9;
    config.mutationRate = 0.1;

    auto ox = make_shared<OrderCrossover>(config.crossoverRate);
    auto swapMut = make_shared<SwapMutation>(config.mutationRate);
    auto tourney = make_shared<TournamentSelection>(3);
    auto repl = make_shared<GenerationalReplacement>(1);  // 1 elite

    GAEngine engine(config, fitnessFn, feasFn, ox, swapMut, tourney, repl);

    // Initialize population with random CVRP permutations
    Population pop;
    for (int i = 0; i < config.populationSize; ++i) {
        pop.addIndividual(make_unique<IntegerChromosome>(codec.createInteger(rng)));
    }
    CHECK(pop.size() == 20);

    // Run generations
    for (int g = 0; g < config.generations; ++g) {
        engine.step(pop, rng);

        // Invariant: population size remains constant
        CHECK(pop.size() == 20);

        // Invariant: every single individual in the population remains a valid CVRP giant tour!
        for (size_t i = 0; i < pop.size(); ++i) {
            const auto& ind = pop.getIndividual(i);
            const auto* ic = dynamic_cast<const IntegerChromosome*>(&ind);
            CHECK(ic != nullptr);
            CHECK(ic->isValid());
            CHECK(ic->isPermutation());
            CHECK(ic->size() == static_cast<size_t>(codec.permutationLength()));

            Routes r = codec.decode(*ic);
            CHECK(r.size() == static_cast<size_t>(numVehicles));
        }
    }
}

void testGAEngineBinaryKeyEvolution() {
    mt19937 rng(123);

    const int numCustomers = 4;
    const int numVehicles = 2;
    CVRPCodec codec(numCustomers, numVehicles);

    FitnessFunction fitnessFn = [&](const Chromosome& chrom) -> double {
        Routes routes;
        if (!codec.tryDecode(chrom, routes)) {
            return 0.0;
        }
        return 10.0;
    };

    GAConfig config;
    config.populationSize = 10;
    config.generations = 5;
    config.crossoverRate = 0.8;
    config.mutationRate = 0.05;

    auto multiPoint = make_shared<MultiPointCrossover>(2, config.crossoverRate);
    auto bitFlip = make_shared<BitFlipMutation>(config.mutationRate);
    auto tourney = make_shared<TournamentSelection>(2);
    auto steady = make_shared<SteadyStateReplacement>(true);

    GAEngine engine(config, fitnessFn, nullptr, multiPoint, bitFlip, tourney, steady);

    Population pop;
    for (int i = 0; i < config.populationSize; ++i) {
        pop.addIndividual(make_unique<BinaryChromosome>(codec.createBinary(rng)));
    }

    for (int g = 0; g < config.generations; ++g) {
        engine.step(pop, rng);
        CHECK(pop.size() == 10);
        for (size_t i = 0; i < pop.size(); ++i) {
            const auto& ind = pop.getIndividual(i);
            const auto* bc = dynamic_cast<const BinaryChromosome*>(&ind);
            CHECK(bc != nullptr);
            Routes r = codec.decode(*bc);
            CHECK(r.size() == static_cast<size_t>(numVehicles));
        }
    }
}

int main() {
    RUN_TEST(testGAEngineCVRPEndToEnd);
    RUN_TEST(testGAEngineBinaryKeyEvolution);

    return finishTests();
}
