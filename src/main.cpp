#include <iostream>
#include "GAConfig.h"
#include "Population.h"

int main() {
    std::cout << "Program started!\n";

    GAConfig config;
    Population population;

    std::cout << "Population size setting: "
              << config.populationSize << '\n';

    std::cout << "Current population: "
              << population.size() << '\n';

    return 0;
}