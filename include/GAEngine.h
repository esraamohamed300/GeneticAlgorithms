#pragma once

#include "GAConfig.h"
#include "Population.h"

#include <functional>
#include <memory>
#include <random>

class CrossoverMethod;
class MutationMethod;
class SelectionMethod;
class ReplacementMethod;

using FitnessFunction =
    std::function<double(const Chromosome&)>;

using FeasibilityFunction =
    std::function<bool(const Chromosome&)>;

class GAEngine {
private:
    GAConfig config;
    FitnessFunction fitnessFunction;
    FeasibilityFunction feasibilityFunction;

    std::shared_ptr<CrossoverMethod> crossover_;
    std::shared_ptr<MutationMethod> mutation_;
    std::shared_ptr<SelectionMethod> selection_;
    std::shared_ptr<ReplacementMethod> replacement_;

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
        std::shared_ptr<CrossoverMethod> crossover,
        std::shared_ptr<MutationMethod> mutation,
        std::shared_ptr<SelectionMethod> selection = nullptr,
        std::shared_ptr<ReplacementMethod> replacement = nullptr
    );

    void setCrossover(std::shared_ptr<CrossoverMethod> crossover);
    void setMutation(std::shared_ptr<MutationMethod> mutation);
    void setSelection(std::shared_ptr<SelectionMethod> selection);
    void setReplacement(std::shared_ptr<ReplacementMethod> replacement);

    const CrossoverMethod* getCrossover() const { return crossover_.get(); }
    const MutationMethod* getMutation() const { return mutation_.get(); }
    const SelectionMethod* getSelection() const { return selection_.get(); }
    const ReplacementMethod* getReplacement() const { return replacement_.get(); }

    const GAConfig& getConfig() const { return config; }
    const FitnessFunction& getFitnessFunction() const { return fitnessFunction; }
    const FeasibilityFunction& getFeasibilityFunction() const { return feasibilityFunction; }

    // Performs one generational step on the given population.
    void step(Population& population, std::mt19937& rng);
};