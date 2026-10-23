#pragma once

#include "Chromosome.h"

#include <cstddef>
#include <utility>
#include <vector>

// Common base for every chromosome that is stored as a flat sequence of genes.
//
// Binary, Integer and Floating-point chromosomes all derive from this class so
// that crossover / mutation operators (Member 4) can be written once as
// templates over the gene type instead of once per representation.
//
// Operators that need to build children can call clone() and then overwrite
// genes through operator[] / genes(); element access is NOT validated, so call
// isValid() afterwards if you want to check the result.
template <typename T>
class GeneChromosome : public Chromosome {
public:
    using GeneType = T;

    GeneChromosome() = default;
    explicit GeneChromosome(std::vector<T> genes) : genes_(std::move(genes)) {}

    std::size_t size() const { return genes_.size(); }

    T& operator[](std::size_t index) { return genes_[index]; }
    const T& operator[](std::size_t index) const { return genes_[index]; }

    // Bounds-checked access (throws std::out_of_range).
    T& at(std::size_t index) { return genes_.at(index); }
    const T& at(std::size_t index) const { return genes_.at(index); }

    std::vector<T>& genes() { return genes_; }
    const std::vector<T>& genes() const { return genes_; }

    // True when every gene obeys the rules of the concrete representation.
    virtual bool isValid() const = 0;

protected:
    std::vector<T> genes_;
};
