#pragma once

#include "Population.h"

#include <cstddef>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// Survivor replacement
// ----------------------------------------------------------------------------
// Decides who makes it into the next generation, given the current population
// ("parents") and the newly created offspring.
//
// FITNESS CONVENTION: HIGHER fitness = BETTER (same as Selection.h).
//
// The population size never changes: the next generation has exactly as many
// individuals as the current one.

// One slot of the next generation: either parent `index` or offspring `index`.
struct Survivor {
    bool fromOffspring;
    std::size_t index;
};

class ReplacementMethod {
public:
    virtual ~ReplacementMethod() = default;

    // Pure decision logic, no chromosomes involved: returns exactly
    // parentFitness.size() survivors. Easy to unit test.
    // Throws std::invalid_argument if parentFitness is empty or any fitness
    // value is NaN/infinite.
    virtual std::vector<Survivor> chooseSurvivors(
        const std::vector<double>& parentFitness,
        const std::vector<double>& offspringFitness) const = 0;

    // Applies chooseSurvivors() to a real population: afterwards `population`
    // and `fitness` hold the next generation (cloned, so `offspring` is left
    // untouched). `fitness[i]` always belongs to `population[i]`.
    // Throws std::invalid_argument if a fitness vector does not match the
    // size of its population.
    void replace(Population& population,
                 std::vector<double>& fitness,
                 const Population& offspring,
                 const std::vector<double>& offspringFitness) const;

    virtual std::string name() const = 0;

protected:
    static void validate(const std::vector<double>& parentFitness,
                         const std::vector<double>& offspringFitness);
};

// Generational replacement: the offspring become the next generation.
// `elitism` best parents are copied over unchanged first (default 1), and the
// remaining slots are filled with the best offspring. If there are fewer
// offspring than free slots, the best remaining parents fill the gap.
// elitism = 0 gives pure generational replacement.
class GenerationalReplacement : public ReplacementMethod {
public:
    explicit GenerationalReplacement(std::size_t elitism = 1);

    std::vector<Survivor> chooseSurvivors(
        const std::vector<double>& parentFitness,
        const std::vector<double>& offspringFitness) const override;

    std::size_t elitism() const { return elitism_; }
    std::string name() const override { return "Generational"; }

private:
    std::size_t elitism_;
};

// Steady-state replacement: only a few offspring are produced per generation
// and they replace the WORST parents; everyone else survives. The best
// offspring replaces the worst parent, the second best the second worst, etc.
// With onlyIfBetter = true (default) an offspring is only admitted if it is
// strictly fitter than the parent it would replace, so the population never
// gets worse. With false, offspring replace the worst parents unconditionally.
// At most as many parents are replaced as there are offspring.
class SteadyStateReplacement : public ReplacementMethod {
public:
    explicit SteadyStateReplacement(bool onlyIfBetter = true);

    std::vector<Survivor> chooseSurvivors(
        const std::vector<double>& parentFitness,
        const std::vector<double>& offspringFitness) const override;

    bool onlyIfBetter() const { return onlyIfBetter_; }
    std::string name() const override { return "SteadyState"; }

private:
    bool onlyIfBetter_;
};
