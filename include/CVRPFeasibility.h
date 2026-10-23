#pragma once

#include "CVRPCodec.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>
using namespace std;

struct FeasibilityReport {
    bool feasible = false;

    bool routeCountOk = true;
    bool customersOk = true;
    bool capacityOk = true;

    vector<double> routeLoads;              
    vector<int> overloadedRoutes;  
    double totalExcess = 0.0;                   

    vector<int> missingCustomers;
    vector<int> duplicatedCustomers;
    vector<int> invalidIds;                 

    string summary() const;
};


class CVRPFeasibilityChecker {
public:
    
    CVRPFeasibilityChecker(const CVRPCodec& codec,
                           vector<double> demands,
                           double capacity);

    double capacity() const { return capacity_; }
    const vector<double>& demands() const { return demands_; }

    FeasibilityReport check(const Routes& routes) const;
    bool isFeasible(const Routes& routes) const;

    double routeLoad(const Route& route) const;

    
    bool isFeasible(const Chromosome& chromosome) const;

    
    function<bool(const Chromosome&)> asFunction() const;

   
    bool hasUnservableCustomer() const;

private:
    CVRPCodec codec_;
    vector<double> demands_;
    double capacity_;
};
