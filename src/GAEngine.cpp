#include "GAEngine.h"

#include "Crossover.h"
#include "Mutation.h"
#include "Replacement.h"
#include "Selection.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

struct BestTracker {
    unique_ptr<Chromosome> best;
    double fitness = 0.0;
    bool feasible = false;

    void offer(const Chromosome& candidate, double candidateFitness,
               bool candidateFeasible) {
        const bool better =
            !best ||
            (candidateFeasible && !feasible) ||
            (candidateFeasible == feasible && candidateFitness > fitness);
        if (better) {
            best = candidate.clone();
            fitness = candidateFitness;
            feasible = candidateFeasible;
        }
    }
};

constexpr double kRateTolerance = 1e-12;

void requireSameRate(const char* what, double operatorRate, double configRate) {
    if (fabs(operatorRate - configRate) > kRateTolerance) {
        throw invalid_argument(
            string("GAEngine: the ") + what + " operator uses rate " +
            to_string(operatorRate) + " but the config says " +
            to_string(configRate) +
            ". Build the operator with the config's rate (or change the config)."
        );
    }
}

bool policyUsesFeasibility(const GAConfig& config) {
    return config.infeasibilityPolicy != InfeasibilityPolicy::None;
}



GAEngine::GAEngine(
    const GAConfig& config,
    FitnessFunction fitness,
    FeasibilityFunction feasibility
)
    : config(config),
      fitnessFunction(move(fitness)),
      feasibilityFunction(move(feasibility))
{
    this->config.validate();
    if (!fitnessFunction) {
        throw invalid_argument("GAEngine: a fitness function is required.");
    }
}

GAEngine::GAEngine(
    const GAConfig& config,
    FitnessFunction fitness,
    FeasibilityFunction feasibility,
    shared_ptr<CrossoverMethod> crossover,
    shared_ptr<MutationMethod> mutation,
    shared_ptr<SelectionMethod> selection,
    shared_ptr<ReplacementMethod> replacement
)
    : GAEngine(config, move(fitness), move(feasibility))
{
    setCrossover(move(crossover));
    setMutation(move(mutation));
    selection_ = move(selection);
    replacement_ = move(replacement);
}

void GAEngine::setCrossover(shared_ptr<CrossoverMethod> crossover) {
    if (crossover) {
        requireSameRate("crossover", crossover->crossoverRate(),
                        config.crossoverRate);
    }
    crossover_ = move(crossover);
}

void GAEngine::setMutation(shared_ptr<MutationMethod> mutation) {
    if (mutation) {
        requireSameRate("mutation", mutation->mutationRate(),
                        config.mutationRate);
    }
    mutation_ = move(mutation);
}

void GAEngine::setSelection(shared_ptr<SelectionMethod> selection) {
    selection_ = move(selection);
}

void GAEngine::setReplacement(shared_ptr<ReplacementMethod> replacement) {
    replacement_ = move(replacement);
}

Population GAEngine::createInitialPopulation(
    const IndividualFactory& factory,
    mt19937& rng
) const {
    if (!factory) {
        throw invalid_argument(
            "GAEngine::createInitialPopulation: the factory is empty."
        );
    }

    Population population;
    for (int i = 0; i < config.populationSize; ++i) {
        unique_ptr<Chromosome> individual = factory(rng);
        if (!individual) {
            throw invalid_argument(
                "GAEngine::createInitialPopulation: the factory returned null."
            );
        }
        population.addIndividual(move(individual));
    }
    return population;
}



vector<double> GAEngine::applyInfeasibilityPolicy(
    const vector<double>& rawFitness,
    const vector<bool>& feasible,
    const GAConfig& config
) {
    if (rawFitness.size() != feasible.size()) {
        throw invalid_argument(
            "GAEngine::applyInfeasibilityPolicy: fitness and feasibility "
            "vectors differ in size."
        );
    }

    vector<double> effective = rawFitness;
    if (!policyUsesFeasibility(config)) {
        return effective;
    }

    bool anyFeasible = false;
    bool anyInfeasible = false;
    double minFeasible = numeric_limits<double>::infinity();
    double maxInfeasible = -numeric_limits<double>::infinity();
    double minAll = numeric_limits<double>::infinity();
    double maxAll = -numeric_limits<double>::infinity();

    for (int i = 0; i < rawFitness.size(); ++i) {
        minAll = min(minAll, rawFitness[i]);
        maxAll = max(maxAll, rawFitness[i]);
        if (feasible[i]) {
            anyFeasible = true;
            minFeasible = min(minFeasible, rawFitness[i]);
        } else {
            anyInfeasible = true;
            maxInfeasible = max(maxInfeasible, rawFitness[i]);
        }
    }

    if (!anyInfeasible) {
        return effective;
    }

    double penalty = 0.0;
    if (config.infeasiblePenalty > 0.0) {
        penalty = config.infeasiblePenalty;  
    } else {
        
        if (!anyFeasible || maxInfeasible < minFeasible) {
            return effective;
        }
        const double span = maxAll - minAll;
        const double margin =
            span > 0.0 ? span : max(1.0, fabs(minFeasible));
        penalty = (maxInfeasible - minFeasible) + margin;
    }

    for (int i = 0; i < effective.size(); ++i) {
        if (!feasible[i]) {
            effective[i] -= penalty;
        }
    }
    return effective;
}



void GAEngine::checkReady(const Population& population,
                          const char* caller) const {
    const string prefix = string("GAEngine::") + caller + ": ";
    if (population.size() == 0) {
        throw invalid_argument(prefix + "population is empty.");
    }
    if (!selection_) {
        throw invalid_argument(prefix + "selection method is not set.");
    }
    if (!replacement_) {
        throw invalid_argument(prefix + "replacement method is not set.");
    }
}

void GAEngine::evaluate(const Chromosome& individual, bool needFeasibility,
                        double& fitness, bool& feasible) const {
    fitness = fitnessFunction(individual);
    if (!isfinite(fitness)) {
        throw invalid_argument(
            "GAEngine: the fitness function returned a non-finite value "
            "(NaN or infinity)."
        );
    }
    feasible = needFeasibility ? feasibilityFunction(individual) : true;
}



void GAEngine::evolve(Population& population, vector<double>& rawFitness,
                      vector<bool>& feasible, mt19937& rng,
                      BestTracker* tracker) {
    const int popSize = population.size();

    const bool haveFeasibility = static_cast<bool>(feasibilityFunction);
    const bool needFeasibility =
        haveFeasibility && (policyUsesFeasibility(config) || tracker != nullptr);
    const bool reject =
        haveFeasibility &&
        config.infeasibilityPolicy == InfeasibilityPolicy::Reject;
    const int maxAttempts = reject ? config.maxRejectAttempts : 1;

   
    const vector<double> parentSelectionFitness =
        applyInfeasibilityPolicy(rawFitness, feasible, config);

    Population offspring;
    vector<double> offspringFitness;
    vector<bool> offspringFeasible;
    offspringFitness.reserve(popSize);
    offspringFeasible.reserve(popSize);

    while (offspring.size() < popSize) {
        const bool needSecond = offspring.size() + 1 < popSize;

        unique_ptr<Chromosome> c1;
        unique_ptr<Chromosome> c2;
        double fit1 = 0.0, fit2 = 0.0;
        bool ok1 = true, ok2 = true;

        for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
            const int p1Idx = selection_->select(parentSelectionFitness, rng);
            const int p2Idx = selection_->select(parentSelectionFitness, rng);

            const Chromosome& p1 = population.getIndividual(p1Idx);
            const Chromosome& p2 = population.getIndividual(p2Idx);

            if (crossover_) {
                auto children = crossover_->cross(p1, p2, rng);
                c1 = move(children.first);
                c2 = move(children.second);
            } else {
                c1 = p1.clone();
                c2 = p2.clone();
            }

            if (mutation_) {
                mutation_->mutateInPlace(*c1, rng);
                if (needSecond) {
                    mutation_->mutateInPlace(*c2, rng);
                }
            }

            evaluate(*c1, needFeasibility, fit1, ok1);
            if (tracker) tracker->offer(*c1, fit1, ok1);

            ok2 = true;
            if (needSecond) {
                evaluate(*c2, needFeasibility, fit2, ok2);
                if (tracker) tracker->offer(*c2, fit2, ok2);
            }

            
            if (ok1 && ok2) {
                break;
            }
        }

        offspring.addIndividual(move(c1));
        offspringFitness.push_back(fit1);
        offspringFeasible.push_back(ok1);

        if (needSecond) {
            offspring.addIndividual(move(c2));
            offspringFitness.push_back(fit2);
            offspringFeasible.push_back(ok2);
        }
    }

    
    vector<double> poolFitness = rawFitness;
    poolFitness.insert(poolFitness.end(), offspringFitness.begin(),
                       offspringFitness.end());
    vector<bool> poolFeasible = feasible;
    poolFeasible.insert(poolFeasible.end(), offspringFeasible.begin(),
                        offspringFeasible.end());

    const vector<double> poolEffective =
        applyInfeasibilityPolicy(poolFitness, poolFeasible, config);
    const vector<double> parentEffective(poolEffective.begin(),
                                         poolEffective.begin() + popSize);
    const vector<double> offspringEffective(poolEffective.begin() + popSize,
                                            poolEffective.end());

    const vector<Survivor> survivors =
        replacement_->chooseSurvivors(parentEffective, offspringEffective);
    if (survivors.size() != popSize) {
        throw logic_error(
            "GAEngine: the replacement method must keep the population size."
        );
    }

    Population next;
    vector<double> nextFitness;
    vector<bool> nextFeasible;
    nextFitness.reserve(popSize);
    nextFeasible.reserve(popSize);

    for (const Survivor& s : survivors) {
        if (s.fromOffspring) {
            next.addIndividual(offspring.getIndividual(s.index).clone());
            nextFitness.push_back(offspringFitness.at(s.index));
            nextFeasible.push_back(offspringFeasible.at(s.index));
        } else {
            next.addIndividual(population.getIndividual(s.index).clone());
            nextFitness.push_back(rawFitness.at(s.index));
            nextFeasible.push_back(feasible.at(s.index));
        }
    }

    population = move(next);
    rawFitness = move(nextFitness);
    feasible = move(nextFeasible);
}

void GAEngine::step(Population& population, mt19937& rng) {
    checkReady(population, "step");

    const bool needFeasibility =
        static_cast<bool>(feasibilityFunction) && policyUsesFeasibility(config);

    vector<double> rawFitness(population.size());
    vector<bool> feasible(population.size(), true);
    for (int i = 0; i < population.size(); ++i) {
        bool ok = true;
        evaluate(population.getIndividual(i), needFeasibility, rawFitness[i], ok);
        feasible[i] = ok;
    }

    evolve(population, rawFitness, feasible, rng, nullptr);
}

GAResult GAEngine::run(Population& population, mt19937& rng,
                       const GenerationCallback& onGeneration) {
    checkReady(population, "run");
    if (population.size() != static_cast<int>(config.populationSize)) {
        throw invalid_argument(
            "GAEngine::run: the population has " +
            to_string(population.size()) + " individuals but " +
            "config.populationSize is " + to_string(config.populationSize) +
            " (see createInitialPopulation)."
        );
    }

    const bool needFeasibility = static_cast<bool>(feasibilityFunction);

    BestTracker tracker;
    GAResult result;

    vector<double> rawFitness(population.size());
    vector<bool> feasible(population.size(), true);
    for (int i = 0; i < population.size(); ++i) {
        bool ok = true;
        evaluate(population.getIndividual(i), needFeasibility, rawFitness[i], ok);
        feasible[i] = ok;
        tracker.offer(population.getIndividual(i), rawFitness[i], ok);
    }

    result.bestFitnessHistory.reserve(
        static_cast<int>(config.generations) + 1);
    result.bestFitnessHistory.push_back(tracker.fitness);
    if (onGeneration) {
        onGeneration(0, *tracker.best, tracker.fitness, tracker.feasible);
    }

    for (int generation = 1; generation <= config.generations; ++generation) {
        evolve(population, rawFitness, feasible, rng, &tracker);
        result.bestFitnessHistory.push_back(tracker.fitness);
        if (onGeneration) {
            onGeneration(generation, *tracker.best, tracker.fitness,
                         tracker.feasible);
        }
    }

    result.best = move(tracker.best);
    result.bestFitness = tracker.fitness;
    result.bestIsFeasible = tracker.feasible;
    result.generationsRun = config.generations;
    return result;
}
