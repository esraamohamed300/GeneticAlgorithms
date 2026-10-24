#pragma once

#include "GAConfig.h"
#include "Population.h"

#include <functional>
#include <memory>
#include <random>
#include <vector>
using namespace std;

class CrossoverMethod;
class MutationMethod;
class SelectionMethod;
class ReplacementMethod;


using FitnessFunction =
    function<double(const Chromosome&)>;


using FeasibilityFunction =
    function<bool(const Chromosome&)>;

using IndividualFactory =
    function<unique_ptr<Chromosome>(mt19937&)>;


using GenerationCallback = function<void(
    int generation,
    const Chromosome& best,
    double bestFitness,
    bool bestIsFeasible)>;

struct GAResult {
    
    unique_ptr<Chromosome> best;

    double bestFitness = 0.0;

    
    bool bestIsFeasible = false;

    int generationsRun = 0;

    
    vector<double> bestFitnessHistory;
};

struct BestTracker;


class GAEngine {
private:
    GAConfig config;
    FitnessFunction fitnessFunction;
    FeasibilityFunction feasibilityFunction;

    shared_ptr<CrossoverMethod> crossover_;
    shared_ptr<MutationMethod> mutation_;
    shared_ptr<SelectionMethod> selection_;
    shared_ptr<ReplacementMethod> replacement_;

public:
    GAEngine(
        const GAConfig& config,
        FitnessFunction fitness,
        FeasibilityFunction feasibility
    );

    
    GAEngine(
        const GAConfig& config,
        FitnessFunction fitness,
        FeasibilityFunction feasibility,
        shared_ptr<CrossoverMethod> crossover,
        shared_ptr<MutationMethod> mutation,
        shared_ptr<SelectionMethod> selection = nullptr,
        shared_ptr<ReplacementMethod> replacement = nullptr
    );

    void setCrossover(shared_ptr<CrossoverMethod> crossover);
    void setMutation(shared_ptr<MutationMethod> mutation);
    void setSelection(shared_ptr<SelectionMethod> selection);
    void setReplacement(shared_ptr<ReplacementMethod> replacement);

    const CrossoverMethod* getCrossover() const { return crossover_.get(); }
    const MutationMethod* getMutation() const { return mutation_.get(); }
    const SelectionMethod* getSelection() const { return selection_.get(); }
    const ReplacementMethod* getReplacement() const { return replacement_.get(); }

    const GAConfig& getConfig() const { return config; }
    const FitnessFunction& getFitnessFunction() const { return fitnessFunction; }
    const FeasibilityFunction& getFeasibilityFunction() const { return feasibilityFunction; }

    Population createInitialPopulation(const IndividualFactory& factory,
                                       mt19937& rng) const;

    
    void step(Population& population, mt19937& rng);

    
    GAResult run(Population& population, mt19937& rng,
                 const GenerationCallback& onGeneration = nullptr);

    
    static vector<double> applyInfeasibilityPolicy(
        const vector<double>& rawFitness,
        const vector<bool>& feasible,
        const GAConfig& config);

private:
    void checkReady(const Population& population, const char* caller) const;

    void evaluate(const Chromosome& individual, bool needFeasibility,
                  double& fitness, bool& feasible) const;

   
    void evolve(Population& population, vector<double>& rawFitness,
                vector<bool>& feasible, mt19937& rng,
                BestTracker* tracker);
};
