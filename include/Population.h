#pragma once

#include "Chromosome.h"
#include <cstddef>
#include <memory>
#include <vector>
using namespace std;

class Population {
private:
    vector<unique_ptr<Chromosome>> individuals;

public:
    void addIndividual(unique_ptr<Chromosome> individual);

    int size() const;

    Chromosome& getIndividual(int index);
    const Chromosome& getIndividual(int index) const;

    void clear();
};