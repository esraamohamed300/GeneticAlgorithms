#pragma once

#include "GeneChromosome.h"

#include <memory>
#include <random>
#include <string>
#include <vector>
using namespace std;


class FloatChromosome : public GeneChromosome<double> {
public:
    FloatChromosome() = default;

    
    FloatChromosome(vector<double> genes, double lower, double upper);

    static FloatChromosome random(int length, double lower,
                                  double upper, mt19937& rng);

    double lowerBound() const { return lower_; }
    double upperBound() const { return upper_; }

    
    void clamp();

    bool isValid() const override;

    unique_ptr<Chromosome> clone() const override;

    string toString() const;

private:
    double lower_ = 0.0;
    double upper_ = 1.0;
};
