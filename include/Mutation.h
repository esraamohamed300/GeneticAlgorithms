#pragma once

#include "Chromosome.h"

#include <cstddef>
#include <memory>
#include <random>

// ----------------------------------------------------------------------------
// Mutation operators (Member 4)
// ----------------------------------------------------------------------------
// Mutation introduces small random variations into a chromosome.
//
// Invariant rules:
// - mutate() returns a mutated clone, leaving the original intact.
// - mutateInPlace() modifies the chromosome directly.
// - All randomized decisions use the passed-in std::mt19937& rng.
// - Chromosome size, bounds, and representation invariants are strictly preserved.

class MutationMethod {
public:
    virtual ~MutationMethod() = default;

    // Returns a new mutated chromosome (original is unchanged).
    virtual std::unique_ptr<Chromosome> mutate(const Chromosome& individual,
                                               std::mt19937& rng) const = 0;

    // Mutates the chromosome in place.
    virtual void mutateInPlace(Chromosome& individual,
                               std::mt19937& rng) const = 0;

    virtual double mutationRate() const = 0;
};

// ----------------------------------------------------------------------------
// Bit-Flip Mutation
// ----------------------------------------------------------------------------
// Traverses every gene of a BinaryChromosome. Each bit is flipped (0 <-> 1)
// independently with probability `mutationRate`.
//
// Applicable to:
// - BinaryChromosome
// - Rejects any non-binary chromosome with std::invalid_argument.
//
// Safe on empty and single-bit chromosomes.
class BitFlipMutation : public MutationMethod {
private:
    double mutationRate_;

public:
    // Default mutation rate 0.05 (5% chance per bit).
    // Throws std::invalid_argument if mutationRate is not in [0.0, 1.0].
    explicit BitFlipMutation(double mutationRate = 0.05);

    std::unique_ptr<Chromosome> mutate(const Chromosome& individual,
                                       std::mt19937& rng) const override;

    void mutateInPlace(Chromosome& individual,
                       std::mt19937& rng) const override;

    double mutationRate() const override { return mutationRate_; }
};

// ----------------------------------------------------------------------------
// Swap Mutation
// ----------------------------------------------------------------------------
// With probability `mutationRate`, two distinct positions (i != j) are chosen
// uniformly at random and their genes are swapped.
//
// Permutation preservation:
// - Swapping two genes preserves the multiset of genes, so permutation chromosomes
//   (such as CVRP integer giant tours) remain 100% valid with no missing or duplicate genes.
//
// Applicable to:
// - IntegerChromosome (both permutation and non-permutation)
// - FloatChromosome
// - BinaryChromosome
//
// Safe on chromosomes of length < 2 (mutation has no effect).
class SwapMutation : public MutationMethod {
private:
    double mutationRate_;

public:
    // Default mutation rate 0.05 (5% chance to perform a swap on the chromosome).
    // Throws std::invalid_argument if mutationRate is not in [0.0, 1.0].
    explicit SwapMutation(double mutationRate = 0.05);

    std::unique_ptr<Chromosome> mutate(const Chromosome& individual,
                                       std::mt19937& rng) const override;

    void mutateInPlace(Chromosome& individual,
                       std::mt19937& rng) const override;

    double mutationRate() const override { return mutationRate_; }
};

// ----------------------------------------------------------------------------
// Inversion Mutation
// ----------------------------------------------------------------------------
// With probability `mutationRate`, two cut points i <= j are chosen uniformly
// at random, and the sub-sequence of genes in [i, j] is reversed.
//
// Permutation preservation:
// - Reversing a continuous segment preserves every element, strictly maintaining
//   permutation validity (essential for traveling salesperson and CVRP orderings).
//
// Applicable to:
// - IntegerChromosome (both permutation and non-permutation)
// - FloatChromosome
// - BinaryChromosome
//
// Safe on chromosomes of length < 2 (inversion has no effect).
class InversionMutation : public MutationMethod {
private:
    double mutationRate_;

public:
    // Default mutation rate 0.05 (5% chance to perform an inversion).
    // Throws std::invalid_argument if mutationRate is not in [0.0, 1.0].
    explicit InversionMutation(double mutationRate = 0.05);

    std::unique_ptr<Chromosome> mutate(const Chromosome& individual,
                                       std::mt19937& rng) const override;

    void mutateInPlace(Chromosome& individual,
                       std::mt19937& rng) const override;

    double mutationRate() const override { return mutationRate_; }
};
