#pragma once

#include "GeneChromosome.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace std;


class IntegerChromosome : public GeneChromosome<int> {
public:
    IntegerChromosome() = default;

    
    IntegerChromosome(vector<int> genes, int lower, int upper,
                      bool permutation = false);

    static IntegerChromosome random(int length, int lower, int upper,
                                    mt19937& rng);

    static IntegerChromosome randomPermutation(int lower, int upper,
                                               mt19937& rng);

    int lowerBound() const { return lower_; }
    int upperBound() const { return upper_; }
    bool isPermutation() const { return permutation_; }

    bool isValid() const override;

    unique_ptr<Chromosome> clone() const override;

    string toString() const;

private:
    int lower_ = 0;
    int upper_ = 0;
    bool permutation_ = false;
};
