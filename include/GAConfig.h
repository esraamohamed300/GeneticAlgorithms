#pragma once



enum class InfeasibilityPolicy {
    None,
    Penalty,
    Reject
};

struct GAConfig {
    int populationSize = 100;
    int generations = 100;

    double crossoverRate = 0.9;
    double mutationRate = 0.05;

    InfeasibilityPolicy infeasibilityPolicy = InfeasibilityPolicy::Penalty;

    
    double infeasiblePenalty = 0.0;

    int maxRejectAttempts = 10;

    
    void setParameters(
        int newPopulationSize,
        int newGenerations,
        double newCrossoverRate,
        double newMutationRate
    );

    void validate() const;
};
