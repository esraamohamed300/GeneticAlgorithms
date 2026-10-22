#pragma once

#include "Chromosome.h"
#include <cstddef>
#include <memory>
#include <vector>

class Population {
private:
    std::vector<std::unique_ptr<Chromosome>> individuals;

public:
    //Add a new individual to the population
    void addIndividual(std::unique_ptr<Chromosome> individual);

    // Return the number of individuals
    std::size_t size() const;

    //Access an individual 
    Chromosome& getIndividual(std::size_t index);
    const Chromosome& getIndividual(std::size_t index) const;

    //Remove all individuals
    void clear();
};