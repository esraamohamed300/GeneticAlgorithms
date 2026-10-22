#include "GAConfig.h"
#include <stdexcept>

void GAConfig::setParameters(
    int newPopulationSize,
    int newGenerations,
    double newCrossoverRate,
    double newMutationRate
) {
    if (newPopulationSize <= 0 || newGenerations <= 0) {
        throw std::invalid_argument(
            "Population size and generations must be positive."
        );
    }

    if (newCrossoverRate < 0.0 || newCrossoverRate > 1.0 ||
        newMutationRate < 0.0 || newMutationRate > 1.0) {
        throw std::invalid_argument(
            "Rates must be between 0 and 1."
        );
    }

    populationSize = newPopulationSize;
    generations = newGenerations;
    crossoverRate = newCrossoverRate;
    mutationRate = newMutationRate;
}

void GAConfig::validate() const {
    if (populationSize <= 0 || generations <= 0) {
        throw std::invalid_argument(
            "Population size and generations must be positive."
        );
    }

    if (crossoverRate < 0.0 || crossoverRate > 1.0 ||
        mutationRate < 0.0 || mutationRate > 1.0) {
        throw std::invalid_argument(
            "Rates must be between 0 and 1."
        );
    }
}