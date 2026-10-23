# Genetic Algorithm Toolbox + CVRP

A modular, object-oriented genetic algorithm toolbox written from scratch in C++17
(no GA libraries, no OR-Tools), and a program that uses it to solve the
Capacitated Vehicle Routing Problem (CVRP).

## Build, run, test

Needs a C++17 compiler. With CMake (also works in Visual Studio via *Open Folder*):

```bash
cmake -S . -B build
cmake --build build
./build/ga_cvrp         
cd build && ctest          # runs every test
```

Without CMake (g++):

```bash
g++ -std=c++17 -O2 -Iinclude src/*.cpp -o ga_cvrp && ./ga_cvrp

g++ -std=c++17 -Iinclude -Itests $(ls src/*.cpp | grep -v main.cpp) tests/test_engine.cpp -o test_engine && ./test_engine
```

Every test prints `N checks, 0 failed` on success and exits with a non-zero code otherwise.

## What is in the toolbox

| Part | Files |
|---|---|
| Chromosomes: binary, integer (free or permutation), floating point | `Binary/Integer/FloatChromosome`, `GeneChromosome` |
| Selection: tournament, roulette wheel | `Selection` |
| Crossover: multi-point, order (OX) | `Crossover` |
| Mutation: bit-flip, swap, inversion | `Mutation` |
| Replacement: generational (with elitism), steady-state | `Replacement` |
| GA engine and configuration | `GAEngine`, `GAConfig` |
| CVRP: encoding/decoding, feasibility check, distance + fitness | `CVRPCodec`, `CVRPFeasibility`, `CVRPFitness` |

The engine never looks inside a chromosome. Everything problem-specific is passed in:

```cpp
GAConfig config;                       // every field has a default
config.populationSize = 100;
config.generations = 150;

FitnessFunction fitness = [](const Chromosome& c) { /* higher is better */ };
FeasibilityFunction feasible = [](const Chromosome& c) { /* true = satisfies constraints */ };

GAEngine engine(config, fitness, feasible,
                std::make_shared<OrderCrossover>(config.crossoverRate),
                std::make_shared<InversionMutation>(config.mutationRate),
                std::make_shared<TournamentSelection>(3),
                std::make_shared<GenerationalReplacement>(1));

std::mt19937 rng(42);
Population population = engine.createInitialPopulation(randomIndividual, rng);
GAResult result = engine.run(population, rng);   // result.best, result.bestFitness, result.bestIsFeasible
```

* **Fitness convention:** higher is better. A minimisation problem converts its cost,
  e.g. `fitness = 1 / (1 + distance)`. The fitness function must return a finite number.
* The feasibility function is optional (`nullptr` = no constraints).
* The operator rates must equal `config.crossoverRate` / `config.mutationRate`; the engine
  throws `std::invalid_argument` otherwise, so the config is the single source of truth.

## Infeasible solutions

Chosen with `config.infeasibilityPolicy`:

| Policy | What the engine does |
|---|---|
| `Penalty` (default) | Infeasible individuals are penalised before selection and replacement. With `infeasiblePenalty = 0` (default) the penalty is **adaptive**: every infeasible individual ranks below every feasible one, and the order among infeasible ones is kept; no tuning, independent of the fitness scale. A positive `infeasiblePenalty` subtracts that fixed amount instead. |
| `Reject` | Offspring pairs with an infeasible child are regenerated, at most `maxRejectAttempts` times; leftovers are ranked below feasible ones (adaptive penalty), so a run can never get stuck. |
| `None` | Fitness is used as returned (the fitness function may do its own penalising). |

In every policy `run()` remembers the best **feasible** individual ever evaluated, so a feasible
answer is never lost (`GAResult::bestIsFeasible` says whether one was found at all).
On the toy problem in `tests/test_engine.cpp` the final population is 0% feasible with `None`,
about 82% with `Penalty` and 100% with `Reject`.

## CVRP model

Depot = id 0, customers = 1..n, K vehicles. Every representation stores a permutation of
`1..n` plus `K-1` separator values that cut it into K routes (see `docs/M2_Representations.md`),
so each customer is served exactly once by construction and only the capacity can be violated.
`CVRPFitnessEvaluator` turns total distance into fitness (`1 / (1 + distance + penalty * overload)`)
and prints solutions as `Route #k: c1 c2 ...` followed by `Cost <distance>`.

On the built-in 10-customer benchmark the solver finds the exact optimum (406.91, verified with a
dynamic program over all customer subsets).
