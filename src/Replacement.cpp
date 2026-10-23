#include "Replacement.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace {

// Indices 0..n-1 ordered best-first (descending fitness). Stable, so equal
// fitness keeps the original order.
std::vector<std::size_t> bestFirst(const std::vector<double>& fitness) {
    std::vector<std::size_t> order(fitness.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) {
                         return fitness[a] > fitness[b];
                     });
    return order;
}

}  // namespace

// ---------------------------------------------------------------------------
// ReplacementMethod
// ---------------------------------------------------------------------------

void ReplacementMethod::validate(
    const std::vector<double>& parentFitness,
    const std::vector<double>& offspringFitness
) {
    if (parentFitness.empty()) {
        throw std::invalid_argument(
            "Replacement: the current population has no individuals."
        );
    }
    for (const auto* values : {&parentFitness, &offspringFitness}) {
        for (double f : *values) {
            if (!std::isfinite(f)) {
                throw std::invalid_argument(
                    "Replacement: fitness values must be finite numbers."
                );
            }
        }
    }
}

void ReplacementMethod::replace(
    Population& population,
    std::vector<double>& fitness,
    const Population& offspring,
    const std::vector<double>& offspringFitness
) const {
    if (population.size() != fitness.size()) {
        throw std::invalid_argument(
            "Replacement: population and fitness sizes differ."
        );
    }
    if (offspring.size() != offspringFitness.size()) {
        throw std::invalid_argument(
            "Replacement: offspring and offspring fitness sizes differ."
        );
    }

    const std::vector<Survivor> survivors =
        chooseSurvivors(fitness, offspringFitness);

    if (survivors.size() != population.size()) {
        throw std::logic_error(
            "Replacement: a method must keep the population size constant."
        );
    }

    Population next;
    std::vector<double> nextFitness;
    nextFitness.reserve(survivors.size());

    for (const Survivor& s : survivors) {
        if (s.fromOffspring) {
            next.addIndividual(offspring.getIndividual(s.index).clone());
            nextFitness.push_back(offspringFitness.at(s.index));
        } else {
            next.addIndividual(population.getIndividual(s.index).clone());
            nextFitness.push_back(fitness.at(s.index));
        }
    }

    population = std::move(next);
    fitness = std::move(nextFitness);
}

// ---------------------------------------------------------------------------
// Generational
// ---------------------------------------------------------------------------

GenerationalReplacement::GenerationalReplacement(std::size_t elitism)
    : elitism_(elitism)
{
}

std::vector<Survivor> GenerationalReplacement::chooseSurvivors(
    const std::vector<double>& parentFitness,
    const std::vector<double>& offspringFitness
) const {
    validate(parentFitness, offspringFitness);

    const std::size_t size = parentFitness.size();
    const std::size_t elite = std::min(elitism_, size);

    const std::vector<std::size_t> parents = bestFirst(parentFitness);
    const std::vector<std::size_t> children = bestFirst(offspringFitness);

    std::vector<Survivor> survivors;
    survivors.reserve(size);

    // 1. Elites: the best parents survive unchanged.
    for (std::size_t i = 0; i < elite; ++i) {
        survivors.push_back({false, parents[i]});
    }

    // 2. Best offspring fill the free slots.
    for (std::size_t i = 0; i < children.size() && survivors.size() < size;
         ++i) {
        survivors.push_back({true, children[i]});
    }

    // 3. Too few offspring: next-best parents fill whatever is left.
    for (std::size_t i = elite; survivors.size() < size; ++i) {
        survivors.push_back({false, parents[i]});
    }

    return survivors;
}

// ---------------------------------------------------------------------------
// Steady-state
// ---------------------------------------------------------------------------

SteadyStateReplacement::SteadyStateReplacement(bool onlyIfBetter)
    : onlyIfBetter_(onlyIfBetter)
{
}

std::vector<Survivor> SteadyStateReplacement::chooseSurvivors(
    const std::vector<double>& parentFitness,
    const std::vector<double>& offspringFitness
) const {
    validate(parentFitness, offspringFitness);

    // Everyone survives by default; slot i keeps parent i.
    std::vector<Survivor> survivors(parentFitness.size());
    for (std::size_t i = 0; i < survivors.size(); ++i) {
        survivors[i] = {false, i};
    }

    std::vector<std::size_t> worstFirst = bestFirst(parentFitness);
    std::reverse(worstFirst.begin(), worstFirst.end());
    const std::vector<std::size_t> children = bestFirst(offspringFitness);

    const std::size_t pairs = std::min(worstFirst.size(), children.size());
    for (std::size_t i = 0; i < pairs; ++i) {
        const std::size_t victim = worstFirst[i];
        const std::size_t child = children[i];

        // Children get worse and victims get better as i grows, so once one
        // child is rejected all later ones would be too.
        if (onlyIfBetter_ && !(offspringFitness[child] > parentFitness[victim])) {
            break;
        }
        survivors[victim] = {true, child};
    }
    return survivors;
}
