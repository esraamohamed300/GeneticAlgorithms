#include "CVRPFitness.h"

#include <cmath>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <utility>
using namespace std;

namespace {


CVRPInstance prepareInstance(CVRPInstance instance, double penaltyMultiplier) {
    if (instance.numCustomers < 1) {
        throw invalid_argument(
            "CVRPFitnessEvaluator: need at least 1 customer."
        );
    }
    if (instance.numVehicles < 1) {
        throw invalid_argument(
            "CVRPFitnessEvaluator: need at least 1 vehicle."
        );
    }
    if (!isfinite(penaltyMultiplier) || penaltyMultiplier < 0.0) {
        throw invalid_argument(
            "CVRPFitnessEvaluator: the penalty multiplier must be a finite "
            "number >= 0."
        );
    }

    const int nodes =
        static_cast<int>(instance.numCustomers) + 1;  

    if (instance.demands.size() != nodes) {
        throw invalid_argument(
            "CVRPFitnessEvaluator: demands must have numCustomers + 1 entries "
            "(index 0 is the depot)."
        );
    }

    if (instance.distanceMatrix.empty()) {
        if (instance.coords.size() != nodes) {
            throw invalid_argument(
                "CVRPFitnessEvaluator: give numCustomers + 1 coordinates or a "
                "complete distance matrix."
            );
        }
        for (const Point& p : instance.coords) {
            if (!isfinite(p.x) || !isfinite(p.y)) {
                throw invalid_argument(
                    "CVRPFitnessEvaluator: coordinates must be finite."
                );
            }
        }
        instance.buildDistanceMatrix();
    } else {
        if (instance.distanceMatrix.size() != nodes) {
            throw invalid_argument(
                "CVRPFitnessEvaluator: the distance matrix must be "
                "(numCustomers + 1) x (numCustomers + 1)."
            );
        }
        for (const auto& row : instance.distanceMatrix) {
            if (row.size() != nodes) {
                throw invalid_argument(
                    "CVRPFitnessEvaluator: the distance matrix must be square."
                );
            }
            for (double d : row) {
                if (!isfinite(d) || d < 0.0) {
                    throw invalid_argument(
                        "CVRPFitnessEvaluator: distances must be finite and "
                        ">= 0."
                    );
                }
            }
        }
    }

    return instance;
}

}  

CVRPFitnessEvaluator::CVRPFitnessEvaluator(CVRPInstance instance,
                                           double penaltyMultiplier)
    : instance_(prepareInstance(move(instance), penaltyMultiplier)),
      codec_(instance_.numCustomers, instance_.numVehicles),
      feasibilityChecker_(codec_, instance_.demands, instance_.capacity),
      penaltyMultiplier_(penaltyMultiplier)
{
}

double CVRPFitnessEvaluator::routeDistance(const Route& route) const {
    if (route.empty()) {
        return 0.0;  
    }

    for (int customer : route) {
        if (!codec_.isCustomer(customer)) {
            throw invalid_argument(
                "CVRPFitnessEvaluator: invalid customer id " +
                to_string(customer) + " in route."
            );
        }
    }

    const auto& d = instance_.distanceMatrix;

    double total = d[0][static_cast<int>(route.front())];
    for (int i = 1; i < route.size(); ++i) {
        total += d[static_cast<int>(route[i - 1])]
                  [static_cast<int>(route[i])];
    }
    total += d[static_cast<int>(route.back())][0];
    return total;
}

double CVRPFitnessEvaluator::routeLoad(const Route& route) const {
    return feasibilityChecker_.routeLoad(route);
}

double CVRPFitnessEvaluator::totalDistance(const Routes& routes) const {
    double total = 0.0;
    for (const Route& route : routes) {
        total += routeDistance(route);
    }
    return total;
}

double CVRPFitnessEvaluator::cvrpFitness(const Routes& routes) const {
    return 1.0 / (1.0 + totalDistance(routes));
}

double CVRPFitnessEvaluator::cvrpFitnessPenalized(const Routes& routes) const {
    const FeasibilityReport report = feasibilityChecker_.check(routes);

    
    if (!report.routeCountOk || !report.customersOk) {
        return 0.0;
    }

    
    const double cost =
        totalDistance(routes) + penaltyMultiplier_ * report.totalExcess;
    return 1.0 / (1.0 + cost);
}

bool CVRPFitnessEvaluator::validateSolution(const Routes& routes) const {
    return feasibilityChecker_.isFeasible(routes);
}

void CVRPFitnessEvaluator::printSolution(const Routes& routes,
                                         ostream& os) const {
    const ios_base::fmtflags savedFlags = os.flags();
    const streamsize savedPrecision = os.precision();

   
    int lineNumber = 0;
    for (const Route& route : routes) {
        if (route.empty()) {
            continue;
        }
        os << "Route #" << ++lineNumber << ":";
        for (int customer : route) {
            os << ' ' << customer;
        }
        os << '\n';
    }

    const FeasibilityReport report = feasibilityChecker_.check(routes);
    if (!report.routeCountOk || !report.customersOk) {
        os << "INVALID SOLUTION - " << report.summary() << '\n';
        os.flags(savedFlags);
        os.precision(savedPrecision);
        return;
    }

    os << fixed << setprecision(2);
    os << "Cost " << totalDistance(routes) << '\n';

    os << "\nDetails (depot = 0):\n";
    for (int v = 0; v < routes.size(); ++v) {
        const Route& route = routes[v];
        os << "  Vehicle " << v + 1 << ": ";
        if (route.empty()) {
            os << "unused\n";
            continue;
        }
        os << "0";
        for (int customer : route) {
            os << " -> " << customer;
        }
        os << " -> 0 | load " << routeLoad(route) << " / "
           << instance_.capacity << " | distance " << routeDistance(route)
           << '\n';
    }

    os << "  Vehicles used: " << lineNumber << " of " << instance_.numVehicles
       << '\n';
    os << "  Total distance: " << totalDistance(routes) << '\n';
    os << "  Feasible: "
       << (report.feasible
               ? "YES (every customer served once, no vehicle over capacity)"
               : "NO (" + report.summary() + ")")
       << '\n';

    os.flags(savedFlags);
    os.precision(savedPrecision);
}

FitnessFunction CVRPFitnessEvaluator::makeFitnessFunction(
    bool usePenalty
) const {
    
    const auto self = make_shared<const CVRPFitnessEvaluator>(*this);

    return [self, usePenalty](const Chromosome& chromosome) -> double {
        Routes routes;
        if (!self->codec_.tryDecode(chromosome, routes)) {
            return 0.0;  
        }
        return usePenalty ? self->cvrpFitnessPenalized(routes)
                          : self->cvrpFitness(routes);
    };
}
