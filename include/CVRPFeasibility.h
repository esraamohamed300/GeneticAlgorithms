#pragma once

#include "CVRPCodec.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

// Detailed outcome of a feasibility check (useful for penalty functions,
// reports and debugging).
struct FeasibilityReport {
    // True only when every condition below holds.
    bool feasible = false;

    // At most numVehicles routes were given.
    bool routeCountOk = true;
    // Every customer 1..n is served exactly once and no other id appears.
    bool customersOk = true;
    // No route carries more than the vehicle capacity.
    bool capacityOk = true;

    std::vector<double> routeLoads;              // total demand per route
    std::vector<std::size_t> overloadedRoutes;   // indices of routes over capacity
    double totalExcess = 0.0;                    // sum of (load - capacity) over overloaded routes

    std::vector<int> missingCustomers;
    std::vector<int> duplicatedCustomers;
    std::vector<int> invalidIds;                 // ids outside 1..n (the depot 0 is not allowed inside a route)

    // One-line human readable summary, e.g. "infeasible: route 2 overloaded by 4".
    std::string summary() const;
};

// Capacity / serve-once feasibility check for the CVRP.
//
// Instance data it needs:
//   demands  : size n+1, demands[i] = demand of customer i (i = 1..n);
//              demands[0] is the depot and must be 0.
//   capacity : maximum load of every vehicle (identical fleet).
//
// A solution (set of routes) is feasible iff
//   1. it has at most numVehicles routes,
//   2. each customer is served exactly once (no missing, duplicated or
//      unknown ids), and
//   3. the total demand of every route is <= capacity.
class CVRPFeasibilityChecker {
public:
    // Throws std::invalid_argument if demands.size() != numCustomers + 1,
    // demands[0] != 0, any demand is negative/non-finite, or capacity <= 0.
    CVRPFeasibilityChecker(const CVRPCodec& codec,
                           std::vector<double> demands,
                           double capacity);

    double capacity() const { return capacity_; }
    const std::vector<double>& demands() const { return demands_; }

    // Full check of a set of routes.
    FeasibilityReport check(const Routes& routes) const;
    bool isFeasible(const Routes& routes) const;

    // Total demand of one route (ids must be valid customers).
    double routeLoad(const Route& route) const;

    // Decodes the chromosome (Integer, Float or Binary) and checks the
    // resulting routes. A chromosome that cannot be decoded for this problem
    // (wrong type, wrong length, broken permutation) is infeasible.
    bool isFeasible(const Chromosome& chromosome) const;

    // The same check packaged as the callable the GA engine expects:
    // std::function<bool(const Chromosome&)>. The returned function keeps its
    // own copy of the checker, so it stays valid after this object dies.
    std::function<bool(const Chromosome&)> asFunction() const;

    // True if some single customer has more demand than a vehicle can carry.
    // Such an instance has no feasible solution at all.
    bool hasUnservableCustomer() const;

private:
    CVRPCodec codec_;
    std::vector<double> demands_;
    double capacity_;
};
