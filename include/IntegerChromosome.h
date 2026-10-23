#pragma once

#include "GeneChromosome.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

// Integer representation.
//
// Valid values : every gene lies in [lowerBound, upperBound] (inclusive).
// Two flavours, chosen at creation time and remembered by the chromosome:
//   * free integers   : genes may repeat (isPermutation() == false)
//   * permutation     : the chromosome holds each value of
//                       [lowerBound, upperBound] exactly once, so
//                       length == upperBound - lowerBound + 1
//                       (isPermutation() == true). Order crossover and
//                       swap/inversion mutation must preserve this.
class IntegerChromosome : public GeneChromosome<int> {
public:
    IntegerChromosome() = default;

    // Throws std::invalid_argument if lower > upper, a gene is out of range,
    // or (when `permutation` is true) the genes are not a permutation of
    // [lower, upper].
    IntegerChromosome(std::vector<int> genes, int lower, int upper,
                      bool permutation = false);

    // Independent uniform random genes in [lower, upper].
    static IntegerChromosome random(std::size_t length, int lower, int upper,
                                    std::mt19937& rng);

    // Random permutation of lower, lower+1, ..., upper.
    static IntegerChromosome randomPermutation(int lower, int upper,
                                               std::mt19937& rng);

    int lowerBound() const { return lower_; }
    int upperBound() const { return upper_; }
    bool isPermutation() const { return permutation_; }

    // Range check; for permutation chromosomes also checks that every value
    // appears exactly once.
    bool isValid() const override;

    std::unique_ptr<Chromosome> clone() const override;

    // e.g. "3 1 2 5 4". Handy for debugging and tests.
    std::string toString() const;

private:
    int lower_ = 0;
    int upper_ = 0;
    bool permutation_ = false;
};
