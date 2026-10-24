

#include "TestUtils.h"

#include "CVRPFitness.h"
#include "Crossover.h"
#include "GAEngine.h"
#include "IntegerChromosome.h"
#include "Mutation.h"
#include "Replacement.h"
#include "Selection.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
using namespace std;


bool close(double a, double b, double tolerance = 1e-9) {
    return fabs(a - b) <= tolerance;
}


CVRPInstance smallInstance() {
    CVRPInstance inst;
    inst.numCustomers = 3;
    inst.numVehicles = 2;
    inst.capacity = 40.0;
    inst.coords = {{0, 0}, {3, 4}, {6, 8}, {3, 0}};
    inst.demands = {0, 10, 20, 30};
    return inst;  
}


CVRPInstance benchmarkInstance() {
    CVRPInstance inst;
    inst.numCustomers = 10;
    inst.numVehicles = 3;
    inst.capacity = 45.0;
    inst.coords = {{50, 50}, {20, 80}, {85, 75}, {30, 20}, {70, 15}, {15, 45},
                   {90, 40}, {40, 90}, {60, 85}, {25, 10}, {80, 25}};
    inst.demands = {0, 12, 15, 10, 14, 8, 11, 9, 13, 7, 14};
    inst.buildDistanceMatrix();
    return inst;
}


void build(const CVRPInstance& instance) {
    CVRPFitnessEvaluator evaluator(instance);
    (void)evaluator;
}

void testRouteDistanceByHand() {
    CVRPFitnessEvaluator ev(smallInstance());

    CHECK(ev.routeDistance({}) == 0.0);          
    CHECK(close(ev.routeDistance({1}), 10.0));   
    CHECK(close(ev.routeDistance({3}), 6.0));    
    CHECK(close(ev.routeDistance({1, 2}), 20.0)); 
    CHECK(close(ev.routeDistance({1, 3}), 12.0));  
    CHECK(close(ev.routeDistance({3, 1}), 12.0));  
    CHECK(close(ev.routeDistance({1, 2, 3}), 5 + 5 + sqrt(73.0) + 3));
}

void testInvalidIdsAreRejected() {
    CVRPFitnessEvaluator ev(smallInstance());
    CHECK_THROWS(ev.routeDistance({0}), invalid_argument);    
    CHECK_THROWS(ev.routeDistance({1, 4}), invalid_argument);  
    CHECK_THROWS(ev.routeDistance({-1}), invalid_argument);
    CHECK_THROWS(ev.routeLoad({9}), invalid_argument);
}

void testLoadsAndTotals() {
    CVRPFitnessEvaluator ev(smallInstance());
    CHECK(ev.routeLoad({}) == 0.0);
    CHECK(ev.routeLoad({1, 2}) == 30.0);
    CHECK(ev.routeLoad({3}) == 30.0);

    Routes feasible = {{1, 2}, {3}};
    CHECK(close(ev.totalDistance(feasible), 26.0));   
    CHECK(ev.totalDistance({{}, {}}) == 0.0);
    CHECK(close(ev.totalDistance({{3}, {1, 2}}), 26.0));  
}

void testPureFitness() {
    CVRPFitnessEvaluator ev(smallInstance());
    CHECK(close(ev.cvrpFitness({{1, 2}, {3}}), 1.0 / 27.0));  
    CHECK(ev.cvrpFitness({{}, {}}) == 1.0);                  

    CHECK(ev.cvrpFitness({{1, 2}, {3}}) > ev.cvrpFitness({{1}, {2, 3}}));
}

void testValidateSolution() {
    CVRPFitnessEvaluator ev(smallInstance());
    CHECK(ev.validateSolution({{1, 2}, {3}}));          
    CHECK(ev.validateSolution({{3, 1}, {2}}));         
    CHECK(!ev.validateSolution({{1, 2, 3}}));           
    CHECK(!ev.validateSolution({{1, 2}, {}}));          
    CHECK(!ev.validateSolution({{1, 2}, {2, 3}}));      
    CHECK(!ev.validateSolution({{1}, {2}, {3}}));       
}

void testPenalizedFitness() {
    CVRPFitnessEvaluator ev(smallInstance(), 100.0);

    Routes ok = {{1, 2}, {3}};
    CHECK(close(ev.cvrpFitnessPenalized(ok), ev.cvrpFitness(ok)));

    Routes overloaded = {{1, 2, 3}, {}};
    const double distance = 5 + 5 + sqrt(73.0) + 3;
    CHECK(close(ev.cvrpFitnessPenalized(overloaded),
                1.0 / (1.0 + distance + 100.0 * 20.0)));
    CHECK(ev.cvrpFitnessPenalized(overloaded) < ev.cvrpFitnessPenalized(ok));

    CVRPFitnessEvaluator ev2(smallInstance(), 100.0);
    Routes lightOverload = {{1, 2, 3}, {}};   
    Routes lighterOverload = {{2, 3}, {1}};  
    CHECK(ev2.cvrpFitnessPenalized(lighterOverload) >
          ev2.cvrpFitnessPenalized(lightOverload));

    CHECK(ev.cvrpFitnessPenalized({{1, 2}, {}}) == 0.0);       
    CHECK(ev.cvrpFitnessPenalized({{1, 2}, {2, 3}}) == 0.0);   
    CHECK(ev.cvrpFitnessPenalized({{1}, {2}, {3}}) == 0.0);    
    CHECK(ev.cvrpFitnessPenalized({{0, 1}, {2, 3}}) == 0.0);   
}

void testZeroPenaltyMultiplier() {
    CVRPFitnessEvaluator ev(smallInstance(), 0.0);
    Routes overloaded = {{1, 2, 3}, {}};
    CHECK(close(ev.cvrpFitnessPenalized(overloaded), ev.cvrpFitness(overloaded)));
}

void testFitnessFunctionAllRepresentations() {
    CVRPFitnessEvaluator ev(smallInstance(), 100.0);
    const CVRPCodec& codec = ev.codec();
    FitnessFunction penalized = ev.makeFitnessFunction(true);
    FitnessFunction pure = ev.makeFitnessFunction(false);

    Routes good = {{1, 2}, {3}};
    Routes bad = {{1, 2, 3}, {}};

    CHECK(close(penalized(codec.encodeInteger(good)), ev.cvrpFitness(good)));
    CHECK(close(penalized(codec.encodeFloat(good)), ev.cvrpFitness(good)));
    CHECK(close(penalized(codec.encodeBinary(good)), ev.cvrpFitness(good)));

    CHECK(close(pure(codec.encodeInteger(bad)), ev.cvrpFitness(bad)));
    CHECK(penalized(codec.encodeInteger(bad)) < pure(codec.encodeInteger(bad)));
    CHECK(close(penalized(codec.encodeInteger(bad)), ev.cvrpFitnessPenalized(bad)));
}

void testFitnessFunctionHandlesUndecodableChromosomes() {
    CVRPFitnessEvaluator ev(smallInstance());
    FitnessFunction fn = ev.makeFitnessFunction(true);

    CHECK(fn(IntegerChromosome({1, 2}, 1, 2, true)) == 0.0); 
    CHECK(fn(BinaryChromosome({0, 1, 1})) == 0.0);

    class Other : public Chromosome {
    public:
        unique_ptr<Chromosome> clone() const override {
            return make_unique<Other>();
        }
    };
    CHECK(fn(Other()) == 0.0);
}

void testFitnessFunctionOutlivesEvaluator() {
    FitnessFunction fn;
    Routes routes = {{1, 2}, {3}};
    IntegerChromosome chromosome;
    {
        CVRPFitnessEvaluator ev(smallInstance());
        fn = ev.makeFitnessFunction(true);
        chromosome = ev.codec().encodeInteger(routes);
    }
    CHECK(close(fn(chromosome), 1.0 / 27.0));
}

void testConstructorValidation() {
    {
        CVRPInstance i = smallInstance();
        i.numCustomers = 0;
        CHECK_THROWS(build(i), invalid_argument);
    }
    {
        CVRPInstance i = smallInstance();
        i.numVehicles = 0;
        CHECK_THROWS(build(i), invalid_argument);
    }
    {
        CVRPInstance i = smallInstance();
        i.demands = {0, 10, 20};  
        CHECK_THROWS(build(i), invalid_argument);
    }
    {
        CVRPInstance i = smallInstance();
        i.capacity = 0.0;
        CHECK_THROWS(build(i), invalid_argument);
    }
    {
        CVRPInstance i = smallInstance();
        i.demands[2] = -5.0;
        CHECK_THROWS(build(i), invalid_argument);
    }
    {
        CVRPInstance i = smallInstance();
        i.coords.pop_back();  
        CHECK_THROWS(build(i), invalid_argument);
    }
    {
        CVRPInstance i = smallInstance();
        i.coords[1].x = numeric_limits<double>::quiet_NaN();
        CHECK_THROWS(build(i), invalid_argument);
    }
    CHECK_THROWS(CVRPFitnessEvaluator(smallInstance(), -1.0), invalid_argument);
    CHECK_THROWS(CVRPFitnessEvaluator(
                     smallInstance(), numeric_limits<double>::infinity()),
                 invalid_argument);
}

void testDistanceMatrixInput() {
    
    CVRPInstance inst;
    inst.numCustomers = 2;
    inst.numVehicles = 1;
    inst.capacity = 100.0;
    inst.demands = {0, 1, 1};
    inst.distanceMatrix = {{0, 7, 9}, {7, 0, 2}, {9, 2, 0}};

    CVRPFitnessEvaluator ev(inst);
    CHECK(close(ev.routeDistance({1, 2}), 7 + 2 + 9));
    CHECK(close(ev.routeDistance({2, 1}), 9 + 2 + 7));

    CVRPInstance wrongSize = inst;
    wrongSize.distanceMatrix = {{0, 7}, {7, 0}};
    CHECK_THROWS(build(wrongSize), invalid_argument);

    CVRPInstance notSquare = inst;
    notSquare.distanceMatrix = {{0, 7, 9}, {7, 0, 2}, {9, 2}};
    CHECK_THROWS(build(notSquare), invalid_argument);

    CVRPInstance negative = inst;
    negative.distanceMatrix[0][1] = -1.0;
    CHECK_THROWS(build(negative), invalid_argument);
}

void testAccessors() {
    CVRPFitnessEvaluator ev(smallInstance());
    CHECK(ev.instance().numCustomers == 3);
    CHECK(ev.instance().distanceMatrix.size() == 4);  
    CHECK(ev.codec().permutationLength() == 4);       
    CHECK(ev.checker().capacity() == 40.0);
    CHECK(close(ev.instance().distanceMatrix[0][1], 5.0));
}

void testPrintSolution() {
    CVRPFitnessEvaluator ev(smallInstance());

    ostringstream out;
    out << scientific << setprecision(5);  
    const ios_base::fmtflags before = out.flags();
    const streamsize precisionBefore = out.precision();
    out.str("");

    ev.printSolution({{1, 2}, {3}}, out);

    CHECK(out.flags() == before);
    CHECK(out.precision() == precisionBefore);

    const string text = out.str();
    CHECK(text.find("Route #1: 1 2\n") != string::npos);
    CHECK(text.find("Route #2: 3\n") != string::npos);
    CHECK(text.find("Cost 26.00") != string::npos);
    CHECK(text.find("0 -> 1 -> 2 -> 0 | load 30.00 / 40.00 | distance 20.00") !=
          string::npos);
    CHECK(text.find("Vehicles used: 2 of 2") != string::npos);
    CHECK(text.find("Feasible: YES") != string::npos);
}

void testPrintSolutionSkipsUnusedVehiclesAndFlagsProblems() {
    CVRPInstance inst = smallInstance();
    inst.capacity = 100.0;
    CVRPFitnessEvaluator ev(inst);

    ostringstream unused;
    ev.printSolution({{}, {2, 1, 3}}, unused);
    CHECK(unused.str().find("Route #1: 2 1 3\n") != string::npos);  
    CHECK(unused.str().find("Route #2") == string::npos);
    CHECK(unused.str().find("Vehicle 1: unused") != string::npos);
    CHECK(unused.str().find("Vehicles used: 1 of 2") != string::npos);

    CVRPFitnessEvaluator tight(smallInstance());
    ostringstream over;
    tight.printSolution({{1, 2, 3}, {}}, over);
    CHECK(over.str().find("Feasible: NO") != string::npos);

    ostringstream invalid;
    ev.printSolution({{1, 9}, {}}, invalid);
    CHECK(invalid.str().find("INVALID SOLUTION") != string::npos);
    CHECK(invalid.str().find("Cost") == string::npos);
}


void testToolboxSolvesBenchmarkCVRP() {
    const double optimum = 406.9144;

    CVRPFitnessEvaluator ev(benchmarkInstance(), 500.0);
    const CVRPCodec& codec = ev.codec();

    GAConfig config;
    config.populationSize = 100;
    config.generations = 150;
    config.crossoverRate = 0.9;
    config.mutationRate = 0.15;

    GAEngine engine(config, ev.makeFitnessFunction(true),
                    ev.checker().asFunction(),
                    make_shared<OrderCrossover>(config.crossoverRate),
                    make_shared<InversionMutation>(config.mutationRate),
                    make_shared<TournamentSelection>(3),
                    make_shared<GenerationalReplacement>(1));

    double bestDistance = numeric_limits<double>::infinity();
    for (unsigned seed = 1; seed <= 10; ++seed) {
        mt19937 rng(seed);
        Population population = engine.createInitialPopulation(
            [&](mt19937& r) {
                return make_unique<IntegerChromosome>(codec.createInteger(r));
            },
            rng);

        GAResult result = engine.run(population, rng);

        Routes routes;
        CHECK(codec.tryDecode(*result.best, routes));
        CHECK(result.bestIsFeasible);
        CHECK(ev.validateSolution(routes));  

        const double distance = ev.totalDistance(routes);
        CHECK(distance >= optimum - 1e-6);  
        bestDistance = min(bestDistance, distance);
    }

    CHECK(bestDistance <= optimum * 1.03);   
}

int main() {
    RUN_TEST(testRouteDistanceByHand);
    RUN_TEST(testInvalidIdsAreRejected);
    RUN_TEST(testLoadsAndTotals);
    RUN_TEST(testPureFitness);
    RUN_TEST(testValidateSolution);
    RUN_TEST(testPenalizedFitness);
    RUN_TEST(testZeroPenaltyMultiplier);
    RUN_TEST(testFitnessFunctionAllRepresentations);
    RUN_TEST(testFitnessFunctionHandlesUndecodableChromosomes);
    RUN_TEST(testFitnessFunctionOutlivesEvaluator);
    RUN_TEST(testConstructorValidation);
    RUN_TEST(testDistanceMatrixInput);
    RUN_TEST(testAccessors);
    RUN_TEST(testPrintSolution);
    RUN_TEST(testPrintSolutionSkipsUnusedVehiclesAndFlagsProblems);
    RUN_TEST(testToolboxSolvesBenchmarkCVRP);

    return finishTests();
}
