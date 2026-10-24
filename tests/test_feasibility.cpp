#include "TestUtils.h"

#include "CVRPFeasibility.h"

#include <limits>
#include <random>
using namespace std;


CVRPCodec makeCodec() { return CVRPCodec(5, 2); }

vector<double> demands() { return {0, 2, 3, 4, 5, 6}; }

CVRPFeasibilityChecker makeChecker(double capacity = 10.0) {
    return CVRPFeasibilityChecker(makeCodec(), demands(), capacity);
}

class OtherChromosome : public Chromosome {
public:
    unique_ptr<Chromosome> clone() const override {
        return make_unique<OtherChromosome>();
    }
};

void testValidAndOverloadedRoutes() {
    auto checker = makeChecker();

    Routes ok = {{1, 2, 4}, {3, 5}};
    CHECK(checker.isFeasible(ok));
    auto report = checker.check(ok);
    CHECK(report.feasible && report.routeCountOk && report.customersOk &&
          report.capacityOk);
    CHECK((report.routeLoads == vector<double>{10, 10}));
    CHECK(report.totalExcess == 0.0);
    CHECK(report.summary() == "feasible");

    Routes bad = {{1, 2, 3}, {4, 5}};
    CHECK(!checker.isFeasible(bad));
    auto badReport = checker.check(bad);
    CHECK(!badReport.feasible);
    CHECK(!badReport.capacityOk);
    CHECK(badReport.customersOk && badReport.routeCountOk);
    CHECK((badReport.overloadedRoutes == vector<int>{1}));
    CHECK(badReport.totalExcess == 1.0);
    CHECK(badReport.summary().find("route 2 overloaded") != string::npos);
}

void testTotalExcessSumsAllOverloadedRoutes() {
    auto checker = makeChecker(5.0);
    Routes routes = {{1, 2}, {3, 4, 5}};
    auto report = checker.check(routes);
    CHECK(!report.feasible);
    CHECK(report.totalExcess == 10.0);

    auto both = checker.check({{1, 2, 3}, {4, 5}});
    CHECK(both.overloadedRoutes.size() == 2);
    CHECK(both.totalExcess == 10.0);
}

void testCustomerServedExactlyOnce() {
    auto checker = makeChecker(100.0);

    auto missing = checker.check({{1, 2, 4}, {3}});
    CHECK(!missing.feasible && !missing.customersOk);
    CHECK((missing.missingCustomers == vector<int>{5}));
    CHECK(missing.capacityOk);

    auto dup = checker.check({{1, 2, 4}, {3, 5, 1}});
    CHECK(!dup.feasible && !dup.customersOk);
    CHECK((dup.duplicatedCustomers == vector<int>{1}));

    auto invalid = checker.check({{0, 1, 2, 4}, {3, 5, 6}});
    CHECK(!invalid.feasible && !invalid.customersOk);
    CHECK((invalid.invalidIds == vector<int>{0, 6}));

    CHECK(checker.isFeasible(Routes{{5, 1, 2, 3, 4}}));
    CHECK(checker.isFeasible(Routes{{5, 1, 2, 3, 4}, {}}));
}

void testFleetSizeLimit() {
    auto checker = makeChecker(100.0);
    Routes threeRoutes = {{1, 2}, {3, 4}, {5}};
    auto report = checker.check(threeRoutes);
    CHECK(!report.feasible);
    CHECK(!report.routeCountOk);
    CHECK(report.customersOk && report.capacityOk);
    CHECK(report.summary().find("too many routes") != string::npos);
}

void testDepotOrderAndRouteOrderDoNotMatterForFeasibility() {
    auto checker = makeChecker();
    CHECK(checker.isFeasible(Routes{{4, 2, 1}, {5, 3}}));
    CHECK(checker.isFeasible(Routes{{3, 5}, {1, 4, 2}}));
}

void testFloatingPointLoadsUseTolerance() {
    CVRPCodec codec(3, 1);
    CVRPFeasibilityChecker checker(codec, {0, 0.1, 0.2, 0.7}, 1.0);
    CHECK(checker.isFeasible(Routes{{1, 2, 3}}));

    CVRPFeasibilityChecker tight(codec, {0, 0.1, 0.2, 0.7}, 0.99);
    CHECK(!tight.isFeasible(Routes{{1, 2, 3}}));
}

void testRouteLoad() {
    auto checker = makeChecker();
    CHECK(checker.routeLoad({}) == 0.0);
    CHECK(checker.routeLoad({1, 5}) == 8.0);
    CHECK_THROWS(checker.routeLoad({1, 9}), invalid_argument);
    CHECK_THROWS(checker.routeLoad({0}), invalid_argument);
}

void testConstructorValidation() {
    CVRPCodec codec = makeCodec();
    CHECK_THROWS(CVRPFeasibilityChecker(codec, {0, 1, 2}, 10), invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(codec, {1, 2, 3, 4, 5, 6}, 10),
                 invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(codec, {0, 2, -3, 4, 5, 6}, 10),
                 invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(
                     codec, {0, 2, numeric_limits<double>::quiet_NaN(), 4, 5, 6}, 10),
                 invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(codec, demands(), 0), invalid_argument);
    CHECK_THROWS(CVRPFeasibilityChecker(codec, demands(), -5), invalid_argument);
}

void testUnservableCustomerDetection() {
    CHECK(!makeChecker(10.0).hasUnservableCustomer());
    CHECK(makeChecker(5.5).hasUnservableCustomer());
}

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
    mt19937 rng(321);
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
    CHECK(feasibleCount > 0 && feasibleCount < 300);
}

void testUndecodableChromosomesAreInfeasible() {
    CVRPCodec codec = makeCodec();
    auto checker = makeChecker(100.0);

    CHECK(!checker.isFeasible(IntegerChromosome({1, 2, 3}, 1, 3, true)));
    CHECK(!checker.isFeasible(FloatChromosome({0.1, 0.2}, 0.0, 1.0)));
    CHECK(!checker.isFeasible(BinaryChromosome({0, 1})));

    mt19937 rng(1);
    IntegerChromosome dup = codec.createInteger(rng);
    dup[1] = dup[0];
    CHECK(!checker.isFeasible(dup));

    CHECK(!checker.isFeasible(OtherChromosome()));
}

void testAsFunctionMatchesEngineSignature() {
    function<bool(const Chromosome&)> feasible;
    CVRPCodec codec = makeCodec();
    {
        auto checker = makeChecker();
        feasible = checker.asFunction();
    }

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