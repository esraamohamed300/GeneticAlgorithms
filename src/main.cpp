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
#include <numeric>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <string>
#include <vector>
#include <cstddef>

using namespace std;


CVRPInstance buildBenchmarkCVRP() {
    CVRPInstance inst;
    inst.numCustomers = 10;
    inst.numVehicles = 3;
    inst.capacity = 45.0;

    inst.coords = {
        {50.0, 50.0},  
        {20.0, 80.0},  
        {85.0, 75.0},  
        {30.0, 20.0},  
        {70.0, 15.0},  
        {15.0, 45.0},  
        {90.0, 40.0},  
        {40.0, 90.0},  
        {60.0, 85.0},  
        {25.0, 10.0},  
        {80.0, 25.0}   
    };
   
    inst.demands = {
        0.0,   
        12.0,  
        15.0,  
        10.0,  
        14.0,  
        8.0,   
        11.0,  
        9.0,   
        13.0,  
        7.0,   
        14.0   
    };

    inst.buildDistanceMatrix();
    return inst;
}


template <typename T>
T askValue(const string& label, T defaultValue, T minValue, T maxValue) {
    while (true) {
        cout << "  " << label << " [" << defaultValue << "]: ";
        string line;
        if (!getline(cin, line)) {
            cout << "\n";
            return defaultValue;  
        }
        if (line.find_first_not_of(" \t\r") == string::npos) {
            return defaultValue;  
        }
        try {
            size_t used = 0;
            double parsed = stod(line, &used);
            if (line.find_first_not_of(" \t\r", used) != string::npos) {
                throw invalid_argument("trailing characters");
            }
            T value = static_cast<T>(parsed);
            if (parsed < static_cast<double>(minValue) ||
                parsed > static_cast<double>(maxValue) ||
                (is_integral<T>::value && parsed != static_cast<double>(value))) {
                throw out_of_range("range");
            }
            return value;
        } catch (const exception&) {
            cout << "    Invalid value. Enter a number between " << minValue
                 << " and " << maxValue << ", or press Enter for the default.\n";
        }
    }
}

InfeasibilityPolicy askPolicy(InfeasibilityPolicy defaultPolicy) {
    while (true) {
        cout << "  Infeasible-solution policy: 1 = Penalty, 2 = Reject, 3 = None ["
             << (defaultPolicy == InfeasibilityPolicy::Penalty ? 1
                 : defaultPolicy == InfeasibilityPolicy::Reject ? 2 : 3)
             << "]: ";
        string line;
        if (!getline(cin, line)) {
            cout << "\n";
            return defaultPolicy;
        }
        if (line.find_first_not_of(" \t\r") == string::npos) return defaultPolicy;
        if (line.find('1') != string::npos) return InfeasibilityPolicy::Penalty;
        if (line.find('2') != string::npos) return InfeasibilityPolicy::Reject;
        if (line.find('3') != string::npos) return InfeasibilityPolicy::None;
        cout << "    Please enter 1, 2 or 3 (or press Enter for the default).\n";
    }
}

string policyName(InfeasibilityPolicy p) {
    switch (p) {
        case InfeasibilityPolicy::Penalty: return "Penalty (adaptive: feasible always ranks first)";
        case InfeasibilityPolicy::Reject:  return "Reject (regenerate infeasible offspring)";
        default:                           return "None (fitness used as returned)";
    }
}

int main(int argc, char** argv) {
    
    int trials = 10;
    unsigned seed = 42;
    bool askUser = true;
    try {
        vector<string> positional;
        for (int i = 1; i < argc; ++i) {
            const string arg = argv[i];
            if (arg == "--defaults" || arg == "-d") askUser = false;
            else positional.push_back(arg);
        }
        if (positional.size() > 0) trials = stoi(positional[0]);
        if (positional.size() > 1) seed = static_cast<unsigned>(stoul(positional[1]));
        if (trials < 1) throw invalid_argument("trials must be >= 1");
    } catch (const exception&) {
        cerr << "Usage: " << argv[0] << " [--defaults] [trials >= 1] [seed]\n";
        return 2;
    }

    try {
        cout << "============================================================\n";
        cout << "   GENETIC ALGORITHM TOOLBOX - PART 2: CVRP SOLVER DEMO\n";
        cout << "============================================================\n";

       
        CVRPInstance instance = buildBenchmarkCVRP();
        CVRPFitnessEvaluator evaluator(instance, 500.0);  
        const CVRPCodec& codec = evaluator.codec();

        const double totalDemand = accumulate(
            instance.demands.begin(), instance.demands.end(), 0.0);

        cout << "Problem configuration:\n";
        cout << "  - Customers        : " << instance.numCustomers << "\n";
        cout << "  - Vehicles         : " << instance.numVehicles << "\n";
        cout << "  - Vehicle Capacity : " << instance.capacity << "\n";
        cout << "  - Total Demand     : " << totalDemand << "\n";
        cout << "  - Chromosome Model : Giant Tour Permutation (Length = "
             << codec.permutationLength() << ")\n\n";

        
        GAConfig config;
        config.generations = 150;
        config.mutationRate = 0.15;
        config.infeasibilityPolicy = InfeasibilityPolicy::Penalty;  
        int tournamentSize = 3;
        int elitism = 1;

       
        if (askUser) {
            cout << "GA settings - press Enter to keep the value in [brackets].\n";
            config.populationSize = askValue<int>("Population size", config.populationSize, 2, 100000);
            config.generations = askValue<int>("Generations", config.generations, 1, 1000000);
            config.crossoverRate = askValue<double>("Crossover rate (0-1)", config.crossoverRate, 0.0, 1.0);
            config.mutationRate = askValue<double>("Mutation rate (0-1)", config.mutationRate, 0.0, 1.0);
            tournamentSize = askValue<int>("Tournament size", tournamentSize, 1, config.populationSize);
            elitism = askValue<int>("Elitism (individuals kept each generation)", elitism, 0, config.populationSize);
            config.infeasibilityPolicy = askPolicy(config.infeasibilityPolicy);
            trials = askValue<int>("Number of independent trials", trials, 1, 100000);
            seed = askValue<unsigned>("Random seed", seed, 0u, 4294967295u);
            cout << "\n";
        }
        config.validate();  

        cout << "GA Toolbox Hyperparameters:\n";
        cout << "  - Population Size  : " << config.populationSize << "\n";
        cout << "  - Generations      : " << config.generations << "\n";
        cout << "  - Selection Method : Tournament (Size = " << tournamentSize << ")\n";
        cout << "  - Crossover Method : Order Crossover (OX, Rate = " << config.crossoverRate << ")\n";
        cout << "  - Mutation Method  : Inversion Mutation (Rate = " << config.mutationRate << ")\n";
        cout << "  - Replacement      : Generational with Elitism = " << elitism << "\n";
        cout << "  - Infeasible Sols  : " << policyName(config.infeasibilityPolicy) << "\n";
        cout << "  - Trials / Seed    : " << trials << " / " << seed << "\n";
        cout << "------------------------------------------------------------\n";

        
        auto selection = make_shared<TournamentSelection>(static_cast<int>(tournamentSize));
        auto crossover = make_shared<OrderCrossover>(config.crossoverRate);
        auto mutation = make_shared<InversionMutation>(config.mutationRate);
        auto replacement = make_shared<GenerationalReplacement>(static_cast<int>(elitism));

        FitnessFunction fitnessFn = evaluator.makeFitnessFunction(true);
        FeasibilityFunction feasFn = evaluator.checker().asFunction();

        GAEngine engine(config, fitnessFn, feasFn, crossover, mutation,
                        selection, replacement);

        IndividualFactory randomTour = [&](mt19937& rng) {
            return make_unique<IntegerChromosome>(codec.createInteger(rng));
        };

        
        Routes bestRoutes;
        double bestDistance = 0.0;
        bool haveFeasible = false;
        int feasibleTrials = 0;

        for (int trial = 1; trial <= trials; ++trial) {
            mt19937 rng(seed + static_cast<unsigned>(trial) - 1u);
            Population population = engine.createInitialPopulation(randomTour, rng);

            GenerationCallback progress = nullptr;
            if (trial == 1) {
                cout << "Trial 1 convergence:\n";
                progress = [&](int gen, const Chromosome& best, double fit,
                               bool feasible) {
                    if (gen % 25 != 0 && gen != config.generations) return;
                    Routes r;
                    codec.tryDecode(best, r);
                    cout << "  Gen " << setw(3) << gen
                         << " | Best Distance: " << setw(7) << fixed
                         << setprecision(2) << evaluator.totalDistance(r)
                         << " | Feasible: " << (feasible ? "YES" : "NO ")
                         << " | Best Fitness: " << scientific << setprecision(3)
                         << fit << "\n";
                };
            }

            GAResult result = engine.run(population, rng, progress);

            Routes routes;
            codec.tryDecode(*result.best, routes);
            const double distance = evaluator.totalDistance(routes);
            const bool feasible = result.bestIsFeasible &&
                                  evaluator.validateSolution(routes);

            cout << "Trial " << setw(2) << trial << ": distance "
                 << fixed << setprecision(2) << setw(8) << distance
                 << " | feasible: " << (feasible ? "YES" : "NO") << "\n";

            if (feasible) {
                ++feasibleTrials;
                if (!haveFeasible || distance < bestDistance) {
                    haveFeasible = true;
                    bestDistance = distance;
                    bestRoutes = routes;
                }
            }
        }

        cout << "------------------------------------------------------------\n";
        if (!haveFeasible) {
            cout << "No feasible solution was found in " << trials
                 << " trial(s). Try more generations or a larger population.\n";
            return 1;
        }

        
        cout << "Best feasible solution (" << feasibleTrials << " of " << trials
             << " trials feasible):\n";
        evaluator.printSolution(bestRoutes, cout);
        cout << "============================================================\n";
        return 0;

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}