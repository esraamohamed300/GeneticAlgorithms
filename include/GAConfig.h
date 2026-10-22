#pragma once

struct GAConfig {
    int populationSize = 100;
    int generations = 100;

    double crossoverRate = 0.9;
    double mutationRate = 0.05;

    void setParameters(
        int newPopulationSize,
        int newGenerations,
        double newCrossoverRate,
        double newMutationRate
    );

    void validate() const;
};