#pragma once

#include "Chromosome.h"

#include <cstddef>
#include <memory>
#include <random>

using namespace std;


class MutationMethod {
public:
    virtual ~MutationMethod() = default;

    virtual unique_ptr<Chromosome> mutate(const Chromosome& individual,
                                               mt19937& rng) const = 0;

    virtual void mutateInPlace(Chromosome& individual,
                               mt19937& rng) const = 0;

    virtual double mutationRate() const = 0;
};

class BitFlipMutation : public MutationMethod {
private:
    double mutationRate_;

public:
    
    explicit BitFlipMutation(double mutationRate = 0.05);

    unique_ptr<Chromosome> mutate(const Chromosome& individual,
                                       mt19937& rng) const override;

    void mutateInPlace(Chromosome& individual,
                       mt19937& rng) const override;

    double mutationRate() const override { return mutationRate_; }
};


class SwapMutation : public MutationMethod {
private:
    double mutationRate_;

public:
    
    explicit SwapMutation(double mutationRate = 0.05);

    unique_ptr<Chromosome> mutate(const Chromosome& individual,
                                       mt19937& rng) const override;

    void mutateInPlace(Chromosome& individual,
                       mt19937& rng) const override;

    double mutationRate() const override { return mutationRate_; }
};


class InversionMutation : public MutationMethod {
private:
    double mutationRate_;

public:
    
    explicit InversionMutation(double mutationRate = 0.05);

    unique_ptr<Chromosome> mutate(const Chromosome& individual,
                                       mt19937& rng) const override;

    void mutateInPlace(Chromosome& individual,
                       mt19937& rng) const override;

    double mutationRate() const override { return mutationRate_; }
};
