#include "GAEngine.h"

#include "Crossover.h"
#include "Mutation.h"
#include "Replacement.h"
#include "Selection.h"

#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

GAEngine::GAEngine(
    const GAConfig& config,
    FitnessFunction fitness,
    FeasibilityFunction feasibility
)
    : config(config),
      fitnessFunction(move(fitness)),
      feasibilityFunction(move(feasibility))
{
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
    : config(config),
      fitnessFunction(move(fitness)),
      feasibilityFunction(move(feasibility)),
      crossover_(move(crossover)),
      mutation_(move(mutation)),
      selection_(move(selection)),
      replacement_(move(replacement))
{
}

void GAEngine::setCrossover(shared_ptr<CrossoverMethod> crossover) {
    crossover_ = move(crossover);
}

void GAEngine::setMutation(shared_ptr<MutationMethod> mutation) {
    mutation_ = move(mutation);
}

void GAEngine::setSelection(shared_ptr<SelectionMethod> selection) {
    selection_ = move(selection);
}

void GAEngine::setReplacement(shared_ptr<ReplacementMethod> replacement) {
    replacement_ = move(replacement);
}

void GAEngine::step(Population& population, mt19937& rng) {
    if (population.size() == 0) {
        throw invalid_argument("GAEngine::step: population is empty.");
    }
    if (!selection_) {
        throw invalid_argument("GAEngine::step: selection method is not set.");
    }
    if (!replacement_) {
        throw invalid_argument("GAEngine::step: replacement method is not set.");
    }

    const size_t popSize = population.size();

    // 1. Evaluate parent fitness
    vector<double> parentFitness(popSize);
    for (size_t i = 0; i < popSize; ++i) {
        const auto& ind = population.getIndividual(i);
        parentFitness[i] = fitnessFunction ? fitnessFunction(ind) : 0.0;
    }

    // 2. Produce offspring
    Population offspring;
    vector<double> offspringFitness;
    offspringFitness.reserve(popSize);

    while (offspring.size() < popSize) {
        size_t p1Idx = selection_->select(parentFitness, rng);
        size_t p2Idx = selection_->select(parentFitness, rng);

        const auto& p1 = population.getIndividual(p1Idx);
        const auto& p2 = population.getIndividual(p2Idx);

        unique_ptr<Chromosome> c1;
        unique_ptr<Chromosome> c2;

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
            mutation_->mutateInPlace(*c2, rng);
        }

        double fit1 = fitnessFunction ? fitnessFunction(*c1) : 0.0;
        offspring.addIndividual(move(c1));
        offspringFitness.push_back(fit1);

        if (offspring.size() < popSize) {
            double fit2 = fitnessFunction ? fitnessFunction(*c2) : 0.0;
            offspring.addIndividual(move(c2));
            offspringFitness.push_back(fit2);
        }
    }

    // 3. Survivor replacement
    replacement_->replace(population, parentFitness, offspring, offspringFitness);
}