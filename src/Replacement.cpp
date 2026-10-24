#include "Replacement.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>
using namespace std;

vector<int> bestFirst(const vector<double>& fitness) {
    vector<int> order(fitness.size());
    iota(order.begin(), order.end(), int{0});
    stable_sort(order.begin(), order.end(),
                     [&](int a, int b) {
                         return fitness[a] > fitness[b];
                     });
    return order;
}




void ReplacementMethod::validate(
    const vector<double>& parentFitness,
    const vector<double>& offspringFitness
) {
    if (parentFitness.empty()) {
        throw invalid_argument(
            "Replacement: the current population has no individuals."
        );
    }
    for (const auto* values : {&parentFitness, &offspringFitness}) {
        for (double f : *values) {
            if (!isfinite(f)) {
                throw invalid_argument(
                    "Replacement: fitness values must be finite numbers."
                );
            }
        }
    }
}

void ReplacementMethod::replace(
    Population& population,
    vector<double>& fitness,
    const Population& offspring,
    const vector<double>& offspringFitness
) const {
    if (population.size() != fitness.size()) {
        throw invalid_argument(
            "Replacement: population and fitness sizes differ."
        );
    }
    if (offspring.size() != offspringFitness.size()) {
        throw invalid_argument(
            "Replacement: offspring and offspring fitness sizes differ."
        );
    }

    const vector<Survivor> survivors =
        chooseSurvivors(fitness, offspringFitness);

    if (survivors.size() != population.size()) {
        throw logic_error(
            "Replacement: a method must keep the population size constant."
        );
    }

    Population next;
    vector<double> nextFitness;
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

    population = move(next);
    fitness = move(nextFitness);
}



GenerationalReplacement::GenerationalReplacement(int elitism)
    : elitism_(elitism)
{
}

vector<Survivor> GenerationalReplacement::chooseSurvivors(
    const vector<double>& parentFitness,
    const vector<double>& offspringFitness
) const {
    validate(parentFitness, offspringFitness);

    const int size = parentFitness.size();
    const int elite = min(elitism_, size);

    const vector<int> parents = bestFirst(parentFitness);
    const vector<int> children = bestFirst(offspringFitness);

    vector<Survivor> survivors;
    survivors.reserve(size);

    
    for (int i = 0; i < elite; ++i) {
        survivors.push_back({false, parents[i]});
    }

    
    for (int i = 0; i < children.size() && survivors.size() < size;
         ++i) {
        survivors.push_back({true, children[i]});
    }

  
    for (int i = elite; survivors.size() < size; ++i) {
        survivors.push_back({false, parents[i]});
    }

    return survivors;
}



SteadyStateReplacement::SteadyStateReplacement(bool onlyIfBetter)
    : onlyIfBetter_(onlyIfBetter)
{
}

vector<Survivor> SteadyStateReplacement::chooseSurvivors(
    const vector<double>& parentFitness,
    const vector<double>& offspringFitness
) const {
    validate(parentFitness, offspringFitness);

    
    vector<Survivor> survivors(parentFitness.size());
    for (int i = 0; i < survivors.size(); ++i) {
        survivors[i] = {false, i};
    }

    vector<int> worstFirst = bestFirst(parentFitness);
    reverse(worstFirst.begin(), worstFirst.end());
    const vector<int> children = bestFirst(offspringFitness);

    const int pairs = min(worstFirst.size(), children.size());
    for (int i = 0; i < pairs; ++i) {
        const int victim = worstFirst[i];
        const int child = children[i];

        
        if (onlyIfBetter_ && !(offspringFitness[child] > parentFitness[victim])) {
            break;
        }
        survivors[victim] = {true, child};
    }
    return survivors;
}
