#include "GAEngine.h"

#include <utility>

GAEngine::GAEngine(
    const GAConfig& config,
    FitnessFunction fitness,
    FeasibilityFunction feasibility
)
    : config(config),
      fitnessFunction(std::move(fitness)),
      feasibilityFunction(std::move(feasibility))
{
}