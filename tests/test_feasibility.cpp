// Unit tests for the CVRP feasibility checker.

#include "TestUtils.h"

#include "CVRPFeasibility.h"

#include <limits>
#include <random>

namespace {

// 5 customers, 2 vehicles, capacity 10.
//   customer: 1  2  3  4  5
//   demand:   2  3  4  5  6
CVRPCodec makeCodec() { return CVRPCodec(5, 2); }

std::vector<double> demands() { return {0, 2, 3, 4, 5, 6}; }

CVRPFeasibilityChecker makeChecker(double capacity = 10.0) {
    return CVRPFeasibilityChecker(makeCodec(), demands(), capacity);
}

// A Chromosome that is none of the three representations.
class OtherChromosome : public Chromosome {
public:
    std::unique_ptr<Chromosome> clone() const override {
        return std::make_unique<OtherChromosome>();
    }
};

}  // namespace

// Minimum validation from the task: a route whose total demand exceeds the
// capacity is infeasible; a valid one passes.
void testValidAndOverloadedRoutes() {
    auto checker = makeChecker();

    // Loads 2+3+5 = 10 (exactly the capacity) and 4+6 = 10.
    Routes ok = {{1, 2, 4}, {3, 5}};
    CHECK(checker.isFeasible(ok));
    auto report = checker.check(ok);
    CHECK(report.feasible && report.routeCountOk && report.customersOk &&
          report.capacityOk);
    CHECK((report.routeLoads == std::vector<double>{10, 10}));
    CHECK(report.totalExcess == 0.0);
    CHECK(report.summary() == "feasible");

    // Loads 2+3+4 = 9 and 5+6 = 11 -> second route over by 1.
    Routes bad = {{1, 2, 3}, {4, 5}};
    CHECK(!checker.isFeasible(bad));
    auto badReport = checker.check(bad);
    CHECK(!badReport.feasible);
    CHECK(!badReport.capacityOk);
    CHECK(badReport.customersOk && badReport.routeCountOk);
    CHECK((badReport.overloadedRoutes == std::vector<std::size_t>{1}));
    CHECK(badReport.totalExcess == 1.0);
    CHECK(badReport.summary().find("route 2 overloaded") != std::string::npos);
}

void testTotalExcessSumsAllOverloadedRoutes() {
    auto checker = makeChecker(5.0);  // loads: {1,2} = 5 (ok), {3,4,5} = 15
    Routes routes = {{1, 2}, {3, 4, 5}};
    auto report = checker.check(routes);
    CHECK(!report.feasible);
    CHECK(report.totalExcess == 10.0);

    // Both overloaded: {1,2,3} = 9 over 5 by 4, {4,5} = 11 over 5 by 6.
    auto both = checker.check({{1, 2, 3}, {4, 5}});
    CHECK(both.overloadedRoutes.size() == 2);
    CHECK(both.totalExcess == 10.0);
}

void testCustomerServedExactlyOnce() {
    auto checker = makeChecker(100.0);

    // Missing customer 5.
    auto missing = checker.check({{1, 2, 4}, {3}});
    CHECK(!missing.feasible && !missing.customersOk);
    CHECK((missing.missingCustomers == std::vector<int>{5}));
    CHECK(missing.capacityOk);

    // Customer 1 appears twice.
    auto dup = checker.check({{1, 2, 4}, {3, 5, 1}});
    CHECK(!dup.feasible && !dup.customersOk);
    CHECK((dup.duplicatedCustomers == std::vector<int>{1}));

    // Depot id 0 and unknown id 6 are not allowed inside a route.
    auto invalid = checker.check({{0, 1, 2, 4}, {3, 5, 6}});
    CHECK(!invalid.feasible && !invalid.customersOk);
    CHECK((invalid.invalidIds == std::vector<int>{0, 6}));

    // Everything on one vehicle is fine when capacity allows it.
    CHECK(checker.isFeasible(Routes{{5, 1, 2, 3, 4}}));
    CHECK(checker.isFeasible(Routes{{5, 1, 2, 3, 4}, {}}));  // unused vehicle
}

void testFleetSizeLimit() {
    auto checker = makeChecker(100.0);
    Routes threeRoutes = {{1, 2}, {3, 4}, {5}};  // only 2 vehicles available
    auto report = checker.check(threeRoutes);
    CHECK(!report.feasible);
    CHECK(!report.routeCountOk);
    CHECK(report.customersOk && report.capacityOk);
    CHECK(report.summary().find("too many routes") != std::string::npos);
}

void testDepotOrderAndRouteOrderDoNotMatterForFeasibility() {
    auto checker = makeChecker();
    CHECK(checker.isFeasible(Routes{{4, 2, 1}, {5, 3}}));
    CHECK(checker.isFeasible(Routes{{3, 5}, {1, 4, 2}}));
}

void testFloatingPointLoadsUseTolerance() {
    CVRPCodec codec(3, 1);
    CVRPFeasibilityChecker checker(codec, {0, 0.1, 0.2, 0.7}, 1.0);
    // 0.1 + 0.2 + 0.7 is not exactly 1.0 in binary floating point.
    CHECK(checker.isFeasible(Routes{{1, 2, 3}}));

    CVRPFeasibilityChecker tight(codec, {0, 0.1, 0.2, 0.7}, 0.99);
    CHECK(!tight.isFeasible(Routes{{1, 2, 3}}));
}

void testRouteLoad() {
    auto checker = makeChecker();
    CHECK(checker.routeLoad({}) == 0.0);
    CHECK(checker.routeLoad({1, 5}) == 8.0);
    CHECK_THROWS(checker.routeLoad({1, 9}), std::invalid_argument);
    CHECK_THROWS(checker.routeLoad({0}), std::invalid_argument);
}

void testConstructorValidation() {
    CVRPCodec codec = makeCodec();
    CHECK_THROWS(CVRPFeasibilityChecker(codec, {0, 1, 2}, 10), std::invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(codec, {1, 2, 3, 4, 5, 6}, 10),
                 std::invalid_argument);  // depot demand must be 0
    CHECK_THROWS(CVRPFeasibilityChecker(codec, {0, 2, -3, 4, 5, 6}, 10),
                 std::invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(
                     codec, {0, 2, std::numeric_limits<double>::quiet_NaN(), 4, 5, 6}, 10),
                 std::invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(codec, demands(), 0), std::invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(codec, demands(), -5), std::invalid_argument);
}

void testUnservableCustomerDetection() {
    CHECK(!makeChecker(10.0).hasUnservableCustomer());
    CHECK(makeChecker(5.5).hasUnservableCustomer());  // customer 5 needs 6
}

// -------------------------------------------------- chromosome-level checks --

void testChromosomeFeasibilityAllRepresentations() {
    CVRPCodec codec = makeCodec();
    auto checker = makeChecker();

    Routes good = {{1, 2, 4}, {3, 5}};
    Routes bad = {{1, 2, 3}, {4, 5}};

    CHECK(checker.isFeasible(codec.encodeInteger(good)));
    CHECK(checker.isFeasible(codec.encodeFloat(good)));
    CHECK(checker.isFeasible(codec.encodeBinary(good)));

    CHECK(!checker.isFeasible(codec.encodeInteger(bad)));
    CHECK(!checker.isFeasible(codec.encodeFloat(bad)));
    CHECK(!checker.isFeasible(codec.encodeBinary(bad)));
}

void testChromosomeFeasibilityAgreesWithDecodedRoutes() {
    std::mt19937 rng(321);
    CVRPCodec codec(12, 4);
    CVRPFeasibilityChecker checker(
        codec, {0, 4, 6, 3, 8, 5, 2, 7, 4, 6, 3, 5, 9}, 22.0);

    int feasibleCount = 0;
    for (int i = 0; i < 300; ++i) {
        IntegerChromosome c = codec.createInteger(rng);
        const bool viaChromosome = checker.isFeasible(c);
        CHECK(viaChromosome == checker.isFeasible(codec.decode(c)));
        feasibleCount += viaChromosome;
    }
    CHECK(feasibleCount > 0 && feasibleCount < 300);  // test covers both outcomes
}

void testUndecodableChromosomesAreInfeasible() {
    CVRPCodec codec = makeCodec();
    auto checker = makeChecker(100.0);

    // Wrong length for this problem.
    CHECK(!checker.isFeasible(IntegerChromosome({1, 2, 3}, 1, 3, true)));
    CHECK(!checker.isFeasible(FloatChromosome({0.1, 0.2}, 0.0, 1.0)));
    CHECK(!checker.isFeasible(BinaryChromosome({0, 1})));

    // Corrupted permutation (duplicate) built via raw gene access.
    std::mt19937 rng(1);
    IntegerChromosome dup = codec.createInteger(rng);
    dup[1] = dup[0];
    CHECK(!checker.isFeasible(dup));

    // Not one of the three representations.
    CHECK(!checker.isFeasible(OtherChromosome()));
}

void testAsFunctionMatchesEngineSignature() {
    std::function<bool(const Chromosome&)> feasible;
    CVRPCodec codec = makeCodec();
    {
        auto checker = makeChecker();
        feasible = checker.asFunction();
    }  // checker is gone; the function owns its own copy

    CHECK(feasible(codec.encodeInteger({{1, 2, 4}, {3, 5}})));
    CHECK(!feasible(codec.encodeInteger({{1, 2, 3}, {4, 5}})));
    CHECK(!feasible(OtherChromosome()));
}

int main() {
    RUN_TEST(testValidAndOverloadedRoutes);
    RUN_TEST(testTotalExcessSumsAllOverloadedRoutes);
    RUN_TEST(testCustomerServedExactlyOnce);
    RUN_TEST(testFleetSizeLimit);
    RUN_TEST(testDepotOrderAndRouteOrderDoNotMatterForFeasibility);
    RUN_TEST(testFloatingPointLoadsUseTolerance);
    RUN_TEST(testRouteLoad);
    RUN_TEST(testConstructorValidation);
    RUN_TEST(testUnservableCustomerDetection);
    RUN_TEST(testChromosomeFeasibilityAllRepresentations);
    RUN_TEST(testChromosomeFeasibilityAgreesWithDecodedRoutes);
    RUN_TEST(testUndecodableChromosomesAreInfeasible);
    RUN_TEST(testAsFunctionMatchesEngineSignature);
    return finishTests();
}
