#include "Population.h"

#include <stdexcept>
#include <utility>

//Add new individual to the population
void Population::addIndividual(
    std::unique_ptr<Chromosome> individual
) {
    if (!individual) {
        throw std::invalid_argument(
            "Cannot add a null individual to the population."
        );
    }

    individuals.push_back(std::move(individual));
}

//Return the number of individuals
std::size_t Population::size() const {
    return individuals.size();
}

//Access an individual
Chromosome& Population::getIndividual(std::size_t index) {
    return *individuals.at(index);
}

const Chromosome& Population::getIndividual(
    std::size_t index
) const {
    return *individuals.at(index);
}

//Remove all individuals
void Population::clear() {
    individuals.clear();
}