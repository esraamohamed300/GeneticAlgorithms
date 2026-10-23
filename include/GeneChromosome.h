#pragma once

#include "Chromosome.h"

#include <cstddef>
#include <utility>
#include <vector>
using namespace std;


template <typename T>
class GeneChromosome : public Chromosome {
public:
    using GeneType = T;

    GeneChromosome() = default;
    explicit GeneChromosome(vector<T> genes) : genes_(move(genes)) {}

    int size() const { return genes_.size(); }

    T& operator[](int index) { return genes_[index]; }
    const T& operator[](int index) const { return genes_[index]; }

    T& at(int index) { return genes_.at(index); }
    const T& at(int index) const { return genes_.at(index); }

    vector<T>& genes() { return genes_; }
    const vector<T>& genes() const { return genes_; }

    virtual bool isValid() const = 0;

protected:
    vector<T> genes_;
};
