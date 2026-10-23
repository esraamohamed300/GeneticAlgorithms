#pragma once

#include "Chromosome.h"

#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <utility>

// ----------------------------------------------------------------------------
// Crossover operators (Member 4)
// ----------------------------------------------------------------------------
// Crossover takes two parent chromosomes and produces two offspring.
//
// Invariant rules:
// - Parents are NEVER modified.
// - Offspring have the same size and representation as the parents.
// - If crossover is skipped (according to crossoverRate), the offspring are
//   exact clones of the parents.
// - All randomized decisions use the passed-in std::mt19937& rng.

class CrossoverMethod {
public:
    virtual ~CrossoverMethod() = default;

    // Produces two offspring from two parent chromosomes.
    // Throws std::invalid_argument if parents are incompatible or invalid.
    virtual std::pair<std::unique_ptr<Chromosome>, std::unique_ptr<Chromosome>>
    cross(const Chromosome& parent1, const Chromosome& parent2,
          std::mt19937& rng) const = 0;

    virtual double crossoverRate() const = 0;

};

// ----------------------------------------------------------------------------
// Multi-point Crossover
// ----------------------------------------------------------------------------
// Selects `numPoints` distinct cut points in [1, length - 1], dividing the
// chromosomes into alternating segments. Offspring 1 takes segments
// (P1, P2, P1, ...), and Offspring 2 takes segments (P2, P1, P2, ...).
//
// Applicable to:
// - BinaryChromosome
// - FloatChromosome
// - IntegerChromosome (free integers ONLY, where isPermutation() == false)
//
// Permutation safety:
// - Throws std::invalid_argument if called on IntegerChromosome with
//   isPermutation() == true, because segment swapping duplicates and drops genes.
//   Use OrderCrossover for permutations.
class MultiPointCrossover : public CrossoverMethod {
private:
    std::size_t numPoints_;
    double crossoverRate_;

public:
    // Default 2-point crossover with rate 0.9.
    // Throws std::invalid_argument if numPoints < 1 or crossoverRate not in [0, 1].
    explicit MultiPointCrossover(std::size_t numPoints = 2,
                                 double crossoverRate = 0.9);

    std::pair<std::unique_ptr<Chromosome>, std::unique_ptr<Chromosome>>
    cross(const Chromosome& parent1, const Chromosome& parent2,
          std::mt19937& rng) const override;

    std::size_t numPoints() const { return numPoints_; }
    double crossoverRate() const override { return crossoverRate_; }
};

// ----------------------------------------------------------------------------
// Order Crossover (OX)
// ----------------------------------------------------------------------------
// The standard order crossover operator for permutation chromosomes.
//
// Algorithm:
// 1. Two cut points [cut1, cut2] (inclusive) are chosen uniformly at random.
// 2. Offspring 1 copies the slice [cut1, cut2] directly from Parent 1.
// 3. The remaining slots of Offspring 1 are filled using the genes from Parent 2,
//    starting immediately after cut2 (wrapping around to index 0), preserving
//    their relative sequence and omitting genes already copied from Parent 1.
// 4. Offspring 2 is constructed symmetrically (slice from Parent 2, filled from Parent 1).
//
// Guarantees:
// - Each gene in [lowerBound, upperBound] appears exactly once in each offspring.
// - Permutation validity (no missing, no duplicate genes) is strictly preserved.
//
// Applicable to:
// - IntegerChromosome with isPermutation() == true.
// - Throws std::invalid_argument if parents are not valid permutation chromosomes
//   or have different bounds/lengths.
class OrderCrossover : public CrossoverMethod {
private:
    double crossoverRate_;
public:
    // Default rate 0.9.
    // Throws std::invalid_argument if crossoverRate is not in [0, 1].
    explicit OrderCrossover(double crossoverRate = 0.9);

    std::pair<std::unique_ptr<Chromosome>, std::unique_ptr<Chromosome>>
    cross(const Chromosome& parent1, const Chromosome& parent2,
          std::mt19937& rng) const override;

    double crossoverRate() const override { return crossoverRate_; }

};
