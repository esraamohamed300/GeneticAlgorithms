#pragma once

#include "CVRPCodec.h"
#include "CVRPFeasibility.h"
#include "GAEngine.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// 2D coordinate point
struct Point {
    double x = 0.0;
    double y = 0.0;
};

// ----------------------------------------------------------------------------
// CVRPInstance
// ----------------------------------------------------------------------------
// Holds the problem definition:
// - Depot is at index 0. Customers are at indices 1..numCustomers.
// - Vehicle count and capacity.
// - Coordinates and demands.
// - Precomputed Euclidean distance matrix.
struct CVRPInstance {
    int numCustomers = 0;
    int numVehicles = 0;
    double capacity = 0.0;

    std::vector<Point> coords;        // coords[0] is depot, coords[1..n] are customers
    std::vector<double> demands;      // demands[0] is 0, demands[1..n] are customer demands
    std::vector<std::vector<double>> distanceMatrix;

    // Computes Euclidean distance between two points
    static double distance(const Point& a, const Point& b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    // Builds the (n+1) x (n+1) distance matrix from coordinates
    void buildDistanceMatrix() {
        std::size_t totalNodes = coords.size();
        distanceMatrix.assign(totalNodes, std::vector<double>(totalNodes, 0.0));
        for (std::size_t i = 0; i < totalNodes; ++i) {
            for (std::size_t j = 0; j < totalNodes; ++j) {
                distanceMatrix[i][j] = distance(coords[i], coords[j]);
            }
        }
    }
};

// ----------------------------------------------------------------------------
// CVRPFitnessEvaluator
// ----------------------------------------------------------------------------
// Evaluates route distances, capacity overloads, and converts CVRP
// distance minimization into GA fitness maximization.
class CVRPFitnessEvaluator {
private:
    CVRPInstance instance_;
    CVRPCodec codec_;
    CVRPFeasibilityChecker feasibilityChecker_;
    double penaltyMultiplier_ = 1000.0;

public:
    // Constructor initializes codec, feasibility checker, and builds distance matrix if needed
    CVRPFitnessEvaluator(CVRPInstance instance, double penaltyMultiplier = 1000.0);

    // Calculates distance of one vehicle route: Depot(0) -> c1 -> c2 ... -> Depot(0)
    // Empty routes return distance 0.0
    double routeDistance(const Route& route) const;

    // Calculates the total demand on one route
    double routeLoad(const Route& route) const;

    // Calculates total distance summed over all vehicle routes
    double totalDistance(const Routes& routes) const;

    // Pure fitness (shorter distance gives higher fitness): 1.0 / (1.0 + totalDistance)
    double cvrpFitness(const Routes& routes) const;

    // Penalized fitness: adds penalty for any route capacity violations
    double cvrpFitnessPenalized(const Routes& routes) const;

    // Validates route count, customer coverage, and capacity
    bool validateSolution(const Routes& routes) const;

    // Pretty prints vehicle routes, loads, and total distance
    void printSolution(const Routes& routes, std::ostream& os = std::cout) const;

    // Creates a callable std::function<double(const Chromosome&)> for GAEngine
    FitnessFunction makeFitnessFunction(bool usePenalty = true) const;

    const CVRPInstance& instance() const { return instance_; }
    const CVRPCodec& codec() const { return codec_; }
    const CVRPFeasibilityChecker& checker() const { return feasibilityChecker_; }
};
