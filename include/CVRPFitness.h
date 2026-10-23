#pragma once

#include "CVRPCodec.h"
#include "CVRPFeasibility.h"
#include "GAEngine.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct Point {
    double x = 0.0;
    double y = 0.0;
};


struct CVRPInstance {
    int numCustomers = 0;
    int numVehicles = 0;
    double capacity = 0.0;

    vector<Point> coords;        
    vector<double> demands;      
    vector<vector<double>> distanceMatrix;

    static double distance(const Point& a, const Point& b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        return sqrt(dx * dx + dy * dy);
    }

    void buildDistanceMatrix() {
        int totalNodes = coords.size();
        distanceMatrix.assign(totalNodes, vector<double>(totalNodes, 0.0));
        for (int i = 0; i < totalNodes; ++i) {
            for (int j = 0; j < totalNodes; ++j) {
                distanceMatrix[i][j] = distance(coords[i], coords[j]);
            }
        }
    }
};


class CVRPFitnessEvaluator {
private:
    CVRPInstance instance_;
    CVRPCodec codec_;
    CVRPFeasibilityChecker feasibilityChecker_;
    double penaltyMultiplier_ = 1000.0;

public:
    CVRPFitnessEvaluator(CVRPInstance instance, double penaltyMultiplier = 1000.0);

    
    double routeDistance(const Route& route) const;

    double routeLoad(const Route& route) const;

    double totalDistance(const Routes& routes) const;

    double cvrpFitness(const Routes& routes) const;

    double cvrpFitnessPenalized(const Routes& routes) const;

    bool validateSolution(const Routes& routes) const;

    void printSolution(const Routes& routes, ostream& os = cout) const;

    FitnessFunction makeFitnessFunction(bool usePenalty = true) const;

    const CVRPInstance& instance() const { return instance_; }
    const CVRPCodec& codec() const { return codec_; }
    const CVRPFeasibilityChecker& checker() const { return feasibilityChecker_; }
};
