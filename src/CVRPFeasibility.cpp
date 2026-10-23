#include "CVRPFeasibility.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

// Loads are sums of doubles, so allow a tiny tolerance before calling a
// route overloaded (e.g. 0.1 + 0.2 + 0.7 must fit into capacity 1.0).
constexpr double kEpsilon = 1e-9;

}  // namespace

// ---------------------------------------------------------------------------
// FeasibilityReport
// ---------------------------------------------------------------------------

std::string FeasibilityReport::summary() const {
    if (feasible) {
        return "feasible";
    }

    std::ostringstream out;
    out << "infeasible:";
    if (!routeCountOk) {
        out << " too many routes;";
    }
    for (int id : invalidIds) {
        out << " invalid id " << id << ";";
    }
    for (int id : missingCustomers) {
        out << " customer " << id << " not served;";
    }
    for (int id : duplicatedCustomers) {
        out << " customer " << id << " served more than once;";
    }
    for (std::size_t r : overloadedRoutes) {
        out << " route " << r + 1 << " overloaded (load " << routeLoads[r]
            << ");";
    }
    return out.str();
}

// ---------------------------------------------------------------------------
// CVRPFeasibilityChecker
// ---------------------------------------------------------------------------

CVRPFeasibilityChecker::CVRPFeasibilityChecker(
    const CVRPCodec& codec,
    std::vector<double> demands,
    double capacity
)
    : codec_(codec),
      demands_(std::move(demands)),
      capacity_(capacity)
{
    if (demands_.size() != static_cast<std::size_t>(codec_.numCustomers()) + 1) {
        throw std::invalid_argument(
            "CVRPFeasibilityChecker: demands must have numCustomers + 1 "
            "entries (index 0 is the depot)."
        );
    }
    if (demands_[0] != 0.0) {
        throw std::invalid_argument(
            "CVRPFeasibilityChecker: the depot demand (index 0) must be 0."
        );
    }
    for (double d : demands_) {
        if (!std::isfinite(d) || d < 0.0) {
            throw std::invalid_argument(
                "CVRPFeasibilityChecker: demands must be finite and >= 0."
            );
        }
    }
    if (!std::isfinite(capacity_) || capacity_ <= 0.0) {
        throw std::invalid_argument(
            "CVRPFeasibilityChecker: capacity must be positive."
        );
    }
}

double CVRPFeasibilityChecker::routeLoad(const Route& route) const {
    double load = 0.0;
    for (int customer : route) {
        if (!codec_.isCustomer(customer)) {
            throw std::invalid_argument(
                "CVRPFeasibilityChecker: invalid customer id " +
                std::to_string(customer) + " in route."
            );
        }
        load += demands_[static_cast<std::size_t>(customer)];
    }
    return load;
}

FeasibilityReport CVRPFeasibilityChecker::check(const Routes& routes) const {
    FeasibilityReport report;
    const int n = codec_.numCustomers();

    // 1. Fleet size.
    report.routeCountOk =
        static_cast<int>(routes.size()) <= codec_.numVehicles();

    // 2. Every customer served exactly once; 3. loads.
    std::vector<int> timesServed(static_cast<std::size_t>(n) + 1, 0);
    report.routeLoads.assign(routes.size(), 0.0);

    for (std::size_t r = 0; r < routes.size(); ++r) {
        for (int id : routes[r]) {
            if (!codec_.isCustomer(id)) {
                report.invalidIds.push_back(id);
                continue;
            }
            ++timesServed[static_cast<std::size_t>(id)];
            report.routeLoads[r] += demands_[static_cast<std::size_t>(id)];
        }

        if (report.routeLoads[r] > capacity_ + kEpsilon) {
            report.overloadedRoutes.push_back(r);
            report.totalExcess += report.routeLoads[r] - capacity_;
        }
    }

    for (int id = 1; id <= n; ++id) {
        const int times = timesServed[static_cast<std::size_t>(id)];
        if (times == 0) {
            report.missingCustomers.push_back(id);
        } else if (times > 1) {
            report.duplicatedCustomers.push_back(id);
        }
    }

    report.customersOk = report.invalidIds.empty() &&
                         report.missingCustomers.empty() &&
                         report.duplicatedCustomers.empty();
    report.capacityOk = report.overloadedRoutes.empty();
    report.feasible =
        report.routeCountOk && report.customersOk && report.capacityOk;
    return report;
}

bool CVRPFeasibilityChecker::isFeasible(const Routes& routes) const {
    return check(routes).feasible;
}

bool CVRPFeasibilityChecker::isFeasible(const Chromosome& chromosome) const {
    Routes routes;
    if (!codec_.tryDecode(chromosome, routes)) {
        return false;
    }
    return isFeasible(routes);
}

std::function<bool(const Chromosome&)> CVRPFeasibilityChecker::asFunction() const {
    CVRPFeasibilityChecker copy = *this;
    return [copy](const Chromosome& chromosome) {
        return copy.isFeasible(chromosome);
    };
}

bool CVRPFeasibilityChecker::hasUnservableCustomer() const {
    return std::any_of(demands_.begin(), demands_.end(),
                       [&](double d) { return d > capacity_ + kEpsilon; });
}
