#pragma once

#include "GeneChromosome.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

// Floating-point (real-valued) representation.
//
// Valid values : every gene is a finite double in [lowerBound, upperBound]
//                (inclusive). NaN and infinity are never valid.
//
// Mutation operators that can push a gene outside the range (e.g. Gaussian
// noise) should call clamp() afterwards to repair the chromosome.
class FloatChromosome : public GeneChromosome<double> {
public:
    FloatChromosome() = default;

    // Throws std::invalid_argument if lower > upper or lower/upper are not
    // finite, or any gene is invalid.
    FloatChromosome(std::vector<double> genes, double lower, double upper);

    // Independent uniform random genes in [lower, upper).
    static FloatChromosome random(std::size_t length, double lower,
                                  double upper, std::mt19937& rng);

    double lowerBound() const { return lower_; }
    double upperBound() const { return upper_; }

    // Forces every gene back into [lowerBound, upperBound]; NaN becomes
    // lowerBound. After clamp(), isValid() is always true.
    void clamp();

    bool isValid() const override;

    std::unique_ptr<Chromosome> clone() const override;

    std::string toString() const;

private:
    double lower_ = 0.0;
    double upper_ = 1.0;
};
