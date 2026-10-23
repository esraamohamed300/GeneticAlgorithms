#pragma once

#include "Chromosome.h"

#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <utility>

using namespace std;



class CrossoverMethod {
public:
    virtual ~CrossoverMethod() = default;


    virtual pair<unique_ptr<Chromosome>, unique_ptr<Chromosome>>
    cross(const Chromosome& parent1, const Chromosome& parent2,
          mt19937& rng) const = 0;

    virtual double crossoverRate() const = 0;

};


class MultiPointCrossover : public CrossoverMethod {
private:
    int numPoints_;
    double crossoverRate_;

public:
    
    explicit MultiPointCrossover(int numPoints = 2,
                                 double crossoverRate = 0.9);

    pair<unique_ptr<Chromosome>, unique_ptr<Chromosome>>
    cross(const Chromosome& parent1, const Chromosome& parent2,
          mt19937& rng) const override;

    int numPoints() const { return numPoints_; }
    double crossoverRate() const override { return crossoverRate_; }
};


class OrderCrossover : public CrossoverMethod {
private:
    double crossoverRate_;
public:
   
    explicit OrderCrossover(double crossoverRate = 0.9);

    pair<unique_ptr<Chromosome>, unique_ptr<Chromosome>>
    cross(const Chromosome& parent1, const Chromosome& parent2,
          mt19937& rng) const override;

    double crossoverRate() const override { return crossoverRate_; }

};
