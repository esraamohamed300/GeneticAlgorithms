#include "Population.h"

#include <stdexcept>
#include <utility>
using namespace std;

void Population::addIndividual(
    unique_ptr<Chromosome> individual
) {
    if (!individual) {
        throw invalid_argument(
            "Cannot add a null individual to the population."
        );
    }

    individuals.push_back(move(individual));
}

int Population::size() const {
    return individuals.size();
}

Chromosome& Population::getIndividual(int index) {
    return *individuals.at(index);
}

const Chromosome& Population::getIndividual(
    int index
) const {
    return *individuals.at(index);
}


void Population::clear() {
    individuals.clear();
}