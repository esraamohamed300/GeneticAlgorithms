#pragma once

#include "GAConfig.h"
#include "Population.h"

#include <functional>

using FitnessFunction =
    std::function<double(const Chromosome&)>;

using FeasibilityFunction =
    std::function<bool(const Chromosome&)>;

class GAEngine {
private:
    GAConfig config;
    FitnessFunction fitnessFunction;
    FeasibilityFunction feasibilityFunction;

public:
    GAEngine(
        const GAConfig& config,
        FitnessFunction fitness,
        FeasibilityFunction feasibility
    );
};