#include "CVRPCodec.h"
#include "CVRPFeasibility.h"
#include "CVRPFitness.h"
#include "Crossover.h"
#include "GAConfig.h"
#include "GAEngine.h"
#include "IntegerChromosome.h"
#include "Mutation.h"
#include "Population.h"
#include "Replacement.h"
#include "Selection.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

using namespace std;

// Sets up a benchmark CVRP instance:
// 10 customers + 1 central depot (index 0).
// 3 vehicles, each with capacity 45.0.
CVRPInstance buildBenchmarkCVRP() {
    CVRPInstance inst;
    inst.numCustomers = 10;
    inst.numVehicles = 3;
    inst.capacity = 45.0;

    // (x, y) coordinates for Depot (0) and Customers (1..10)
    inst.coords = {
        {50.0, 50.0},  // 0: Central Depot
        {20.0, 80.0},  // 1: Customer 1
        {85.0, 75.0},  // 2: Customer 2
        {30.0, 20.0},  // 3: Customer 3
        {70.0, 15.0},  // 4: Customer 4
        {15.0, 45.0},  // 5: Customer 5
        {90.0, 40.0},  // 6: Customer 6
        {40.0, 90.0},  // 7: Customer 7
        {60.0, 85.0},  // 8: Customer 8
        {25.0, 10.0},  // 9: Customer 9
        {80.0, 25.0}   // 10: Customer 10
    };

    // Demands for depot (0) and customers (1..10)
    // Total demand = 113. Total vehicle capacity = 3 * 45 = 135.
    inst.demands = {
        0.0,   // 0: Depot
        12.0,  // 1
        15.0,  // 2
        10.0,  // 3
        14.0,  // 4
        8.0,   // 5
        11.0,  // 6
        9.0,   // 7
        13.0,  // 8
        7.0,   // 9
        14.0   // 10
    };

    inst.buildDistanceMatrix();
    return inst;
}

int main() {
    cout << "============================================================\n";
    cout << "   GENETIC ALGORITHM TOOLBOX - PART 2: CVRP SOLVER DEMO\n";
    cout << "============================================================\n";

    // 1. Problem Instance Setup
    CVRPInstance instance = buildBenchmarkCVRP();
    CVRPFitnessEvaluator evaluator(instance, 500.0);  // 500 penalty per overloaded unit
    const CVRPCodec& codec = evaluator.codec();

    cout << "Problem configuration:\n";
    cout << "  - Customers        : " << instance.numCustomers << "\n";
    cout << "  - Vehicles         : " << instance.numVehicles << "\n";
    cout << "  - Vehicle Capacity : " << instance.capacity << "\n";
    cout << "  - Total Demand     : 113.0\n";
    cout << "  - Chromosome Model : Giant Tour Permutation (Length = "
         << codec.permutationLength() << ")\n\n";

    // 2. GA Hyperparameters
    GAConfig config;
    config.populationSize = 100;
    config.generations = 150;
    config.crossoverRate = 0.90;
    config.mutationRate = 0.15;
    

    cout << "GA Toolbox Hyperparameters:\n";
    cout << "  - Population Size  : " << config.populationSize << "\n";
    cout << "  - Generations      : " << config.generations << "\n";
    cout << "  - Selection Method : Tournament (Size = 3)\n";
    cout << "  - Crossover Method : Order Crossover (OX, Rate = " << config.crossoverRate << ")\n";
    cout << "  - Mutation Method  : Inversion Mutation (2-Opt, Rate = " << config.mutationRate << ")\n";
    cout << "  - Replacement      : Generational with Elitism = 1\n";
    cout << "------------------------------------------------------------\n";
    cout << "Starting Genetic Algorithm Optimization...\n\n";

    // 3. Connect Operators and Engine
    auto selection = make_shared<TournamentSelection>(3);
    auto crossover = make_shared<OrderCrossover>(config.crossoverRate);
    auto mutation = make_shared<InversionMutation>(config.mutationRate);
    auto replacement = make_shared<GenerationalReplacement>(1);  // Best individual always survives

    FitnessFunction fitnessFn = evaluator.makeFitnessFunction(true);
    FeasibilityFunction feasFn = evaluator.checker().asFunction();

    GAEngine engine(config, fitnessFn, feasFn, crossover, mutation, selection, replacement);

    // 4. Initialize Population with random CVRP giant tour permutations
    mt19937 rng(42);  // Fixed seed for reproducible university demonstration
    Population population;
    for (int i = 0; i < config.populationSize; ++i) {
        population.addIndividual(make_unique<IntegerChromosome>(codec.createInteger(rng)));
    }

    // Helper to evaluate and find the best individual in current population
    auto findBest = [&](const Population& pop) -> pair<unique_ptr<Chromosome>, double> {
        double bestFit = -1.0;
        size_t bestIdx = 0;
        for (size_t i = 0; i < pop.size(); ++i) {
            double f = fitnessFn(pop.getIndividual(i));
            if (f > bestFit) {
                bestFit = f;
                bestIdx = i;
            }
        }
        return {pop.getIndividual(bestIdx).clone(), bestFit};
    };

    auto [initialBestChrom, initialBestFit] = findBest(population);
    Routes initialRoutes = codec.decode(static_cast<const IntegerChromosome&>(*initialBestChrom));
    double initialDistance = evaluator.totalDistance(initialRoutes);

    cout << "Initial Generation (Gen 0):\n";
    cout << "  Best Distance : " << fixed << setprecision(2) << initialDistance
         << " | Feasible: " << (evaluator.validateSolution(initialRoutes) ? "YES" : "NO") << "\n";
    cout << "------------------------------------------------------------\n";

    // 5. Run Evolutionary Generations
    for (int gen = 1; gen <= config.generations; ++gen) {
        engine.step(population, rng);

        if (gen % 25 == 0 || gen == config.generations) {
            auto [currentBest, fit] = findBest(population);
            Routes r = codec.decode(static_cast<const IntegerChromosome&>(*currentBest));
            double dist = evaluator.totalDistance(r);
            bool feas = evaluator.validateSolution(r);

            cout << "Gen " << setw(3) << gen
                 << " | Best Distance: " << setw(7) << fixed << setprecision(2) << dist
                 << " | Feasible: " << (feas ? "YES" : "NO ")
                 << " | Best Fitness: " << scientific << setprecision(3) << fit << "\n";
        }
    }

    // 6. Decode and Display the Final Best Solution
    auto [finalBestChrom, finalBestFit] = findBest(population);
    Routes bestRoutes = codec.decode(static_cast<const IntegerChromosome&>(*finalBestChrom));
    double finalDistance = evaluator.totalDistance(bestRoutes);

    evaluator.printSolution(bestRoutes, cout);

    double improvement = ((initialDistance - finalDistance) / initialDistance) * 100.0;
    cout << "Optimization Performance:\n";
    cout << "  - Initial Best Distance : " << fixed << setprecision(2) << initialDistance << "\n";
    cout << "  - Final Best Distance   : " << fixed << setprecision(2) << finalDistance << "\n";
    cout << "  - Cost Reduction        : " << fixed << setprecision(1) << improvement << "%\n";
    cout << "============================================================\n";

    return 0;
}