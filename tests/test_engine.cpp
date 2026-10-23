
#include "TestUtils.h"

#include "BinaryChromosome.h"
#include "Crossover.h"
#include "GAEngine.h"
#include "Mutation.h"
#include "Replacement.h"
#include "Selection.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <random>
#include <vector>
using namespace std;


namespace {

constexpr int kBits = 20;
constexpr int kMaxOnes = 12;

int onesIn(const Chromosome& c) {
    const auto& bits = dynamic_cast<const BinaryChromosome&>(c);
    int ones = 0;
    for (int i = 0; i < bits.size(); ++i) {
        ones += bits[i];
    }
    return ones;
}

struct Counters {
    int fitnessCalls = 0;
    int feasibilityCalls = 0;
};

FitnessFunction countingFitness(Counters& counters) {
    return [&counters](const Chromosome& c) {
        ++counters.fitnessCalls;
        return static_cast<double>(onesIn(c));
    };
}

FeasibilityFunction countingFeasibility(Counters& counters) {
    return [&counters](const Chromosome& c) {
        ++counters.feasibilityCalls;
        return onesIn(c) <= kMaxOnes;
    };
}

GAConfig smallConfig(InfeasibilityPolicy policy = InfeasibilityPolicy::Penalty) {
    GAConfig config;
    config.populationSize = 30;
    config.generations = 40;
    config.crossoverRate = 0.9;
    config.mutationRate = 0.05;
    config.infeasibilityPolicy = policy;
    return config;
}

GAEngine makeEngine(const GAConfig& config, FitnessFunction fit,
                    FeasibilityFunction feas, int elitism = 1) {
    return GAEngine(config, move(fit), move(feas),
                    make_shared<MultiPointCrossover>(2, config.crossoverRate),
                    make_shared<BitFlipMutation>(config.mutationRate),
                    make_shared<TournamentSelection>(3),
                    make_shared<GenerationalReplacement>(elitism));
}

IndividualFactory randomBits() {
    return [](mt19937& rng) {
        return make_unique<BinaryChromosome>(
            BinaryChromosome::random(kBits, rng));
    };
}


double feasibleShare(const Population& population) {
    int feasible = 0;
    for (int i = 0; i < population.size(); ++i) {
        feasible += onesIn(population.getIndividual(i)) <= kMaxOnes;
    }
    return static_cast<double>(feasible) / static_cast<double>(population.size());
}

class OtherChromosome : public Chromosome {
public:
    unique_ptr<Chromosome> clone() const override {
        return make_unique<OtherChromosome>();
    }
};

}  



void testConfigDefaultsAndValidation() {
    GAConfig config;  
    CHECK(config.populationSize == 100);
    CHECK(config.generations == 100);
    CHECK(config.crossoverRate == 0.9);
    CHECK(config.mutationRate == 0.05);
    CHECK(config.infeasibilityPolicy == InfeasibilityPolicy::Penalty);
    CHECK(config.infeasiblePenalty == 0.0);
    CHECK(config.maxRejectAttempts == 10);
    config.validate();  

    GAConfig bad = config;
    bad.populationSize = 0;
    CHECK_THROWS(bad.validate(), invalid_argument);
    bad = config;
    bad.generations = -1;
    CHECK_THROWS(bad.validate(), invalid_argument);
    bad = config;
    bad.crossoverRate = 1.5;
    CHECK_THROWS(bad.validate(), invalid_argument);
    bad = config;
    bad.mutationRate = numeric_limits<double>::quiet_NaN();
    CHECK_THROWS(bad.validate(), invalid_argument);
    bad = config;
    bad.infeasiblePenalty = -1.0;
    CHECK_THROWS(bad.validate(), invalid_argument);
    bad = config;
    bad.infeasiblePenalty = numeric_limits<double>::infinity();
    CHECK_THROWS(bad.validate(), invalid_argument);
    bad = config;
    bad.maxRejectAttempts = 0;
    CHECK_THROWS(bad.validate(), invalid_argument);
}

void testConfigSetParameters() {
    GAConfig config;
    config.setParameters(50, 20, 0.7, 0.2);
    CHECK(config.populationSize == 50 && config.generations == 20);
    CHECK(config.crossoverRate == 0.7 && config.mutationRate == 0.2);

    
    CHECK_THROWS(config.setParameters(0, 20, 0.7, 0.2), invalid_argument);
    CHECK_THROWS(config.setParameters(50, 20, 1.7, 0.2), invalid_argument);
    CHECK_THROWS(config.setParameters(50, 20, 0.7, nan("")), invalid_argument);
    CHECK(config.populationSize == 50 && config.crossoverRate == 0.7);
}



void testEngineRequiresFitnessFunctionAndValidConfig() {
    Counters counters;
    CHECK_THROWS(GAEngine(smallConfig(), nullptr, countingFeasibility(counters)),
                 invalid_argument);

    GAConfig bad = smallConfig();
    bad.populationSize = 0;
    CHECK_THROWS(GAEngine(bad, countingFitness(counters), nullptr),
                 invalid_argument);

    
    GAEngine ok(smallConfig(), countingFitness(counters), nullptr);
    CHECK(static_cast<bool>(ok.getFitnessFunction()));
    CHECK(!static_cast<bool>(ok.getFeasibilityFunction()));
}

void testUserFunctionsAreStoredAndCallable() {
    Counters counters;
    GAEngine engine(smallConfig(), countingFitness(counters),
                    countingFeasibility(counters));

    BinaryChromosome allOnes(vector<uint8_t>(kBits, 1));
    BinaryChromosome few(vector<uint8_t>(kBits, 0));
    few[0] = few[1] = 1;

    CHECK(engine.getFitnessFunction()(allOnes) == 20.0);
    CHECK(engine.getFitnessFunction()(few) == 2.0);
    CHECK(!engine.getFeasibilityFunction()(allOnes));   
    CHECK(engine.getFeasibilityFunction()(few));
}

void testOperatorRatesMustMatchConfig() {
    Counters counters;
    GAConfig config = smallConfig();  


    CHECK_THROWS(
        GAEngine(config, countingFitness(counters), nullptr,
                 make_shared<MultiPointCrossover>(2, 0.5),
                 make_shared<BitFlipMutation>(0.05)),
        invalid_argument);
    CHECK_THROWS(
        GAEngine(config, countingFitness(counters), nullptr,
                 make_shared<MultiPointCrossover>(2, 0.9),
                 make_shared<BitFlipMutation>(0.3)),
        invalid_argument);

  
    GAEngine engine(config, countingFitness(counters), nullptr);
    CHECK_THROWS(engine.setCrossover(make_shared<OrderCrossover>(0.1)),
                 invalid_argument);
    CHECK_THROWS(engine.setMutation(make_shared<SwapMutation>(0.5)),
                 invalid_argument);
    CHECK(engine.getCrossover() == nullptr);  
    CHECK(engine.getMutation() == nullptr);

    engine.setCrossover(make_shared<OrderCrossover>(0.9));
    engine.setMutation(make_shared<SwapMutation>(0.05));
    CHECK(engine.getCrossover() != nullptr && engine.getMutation() != nullptr);

 
    engine.setCrossover(nullptr);
    engine.setMutation(nullptr);
    CHECK(engine.getCrossover() == nullptr);
}



void testPolicyNoneLeavesFitnessUntouched() {
    GAConfig config = smallConfig(InfeasibilityPolicy::None);
    vector<double> raw = {5, 9, 1, 7};
    vector<bool> feasible = {false, true, false, true};
    CHECK(GAEngine::applyInfeasibilityPolicy(raw, feasible, config) == raw);
}

void testAdaptivePenaltyPutsInfeasibleBelowFeasible() {
    GAConfig config = smallConfig(InfeasibilityPolicy::Penalty);

  
    vector<double> raw       = {10, 9, 8, 7, 6};
    vector<bool> feasible    = {false, true, false, true, true};
    auto eff = GAEngine::applyInfeasibilityPolicy(raw, feasible, config);


    CHECK(eff[1] == 9 && eff[3] == 7 && eff[4] == 6);

  
    const double worstFeasible = min({eff[1], eff[3], eff[4]});
    CHECK(eff[0] < worstFeasible);
    CHECK(eff[2] < worstFeasible);

    CHECK(eff[0] > eff[2]);
}

void testAdaptivePenaltyIsScaleFree() {
    GAConfig config = smallConfig();
    vector<bool> feasible = {false, true, false, true};

    for (double scale : {1e-9, 1.0, 1e9}) {
        vector<double> raw = {10 * scale, 9 * scale, 8 * scale, 7 * scale};
        auto eff = GAEngine::applyInfeasibilityPolicy(raw, feasible, config);
        CHECK(max(eff[0], eff[2]) < min(eff[1], eff[3]));
        CHECK(eff[1] == raw[1] && eff[3] == raw[3]);
        CHECK(eff[0] > eff[2]);
    }
}

void testAdaptivePenaltyEdgeCases() {
    GAConfig config = smallConfig();

    vector<double> raw = {3, 1, 2};
    CHECK(GAEngine::applyInfeasibilityPolicy(raw, {true, true, true}, config) == raw);

    CHECK(GAEngine::applyInfeasibilityPolicy(raw, {false, false, false}, config) == raw);

    vector<double> sep = {1, 2, 9, 8};
    CHECK(GAEngine::applyInfeasibilityPolicy(sep, {false, false, true, true}, config) == sep);

    auto eq = GAEngine::applyInfeasibilityPolicy({5, 5, 5, 5}, {true, false, true, false}, config);
    CHECK(eq[0] == 5 && eq[2] == 5);
    CHECK(eq[1] < 5 && eq[3] < 5);

    auto neg = GAEngine::applyInfeasibilityPolicy({-1, -2, -3, -4}, {false, true, false, true}, config);
    CHECK(max(neg[0], neg[2]) < min(neg[1], neg[3]));
    CHECK(neg[0] > neg[2]);

    CHECK(GAEngine::applyInfeasibilityPolicy({}, {}, config).empty());

    CHECK_THROWS(GAEngine::applyInfeasibilityPolicy({1, 2}, {true}, config),
                 invalid_argument);
}

void testFixedPenalty() {
    GAConfig config = smallConfig();
    config.infeasiblePenalty = 4.0;

    auto eff = GAEngine::applyInfeasibilityPolicy({10, 9, 8}, {false, true, false}, config);
    CHECK(eff[0] == 6.0 && eff[1] == 9.0 && eff[2] == 4.0);

    config.infeasibilityPolicy = InfeasibilityPolicy::Reject;  
    auto rej = GAEngine::applyInfeasibilityPolicy({10, 9, 8}, {false, true, false}, config);
    CHECK(rej[0] == 6.0 && rej[1] == 9.0);
}


void testFeasibilityFunctionIsCalledByStep() {
    mt19937 rng(1);

    Counters penalty;
    GAEngine engine = makeEngine(smallConfig(InfeasibilityPolicy::Penalty),
                                 countingFitness(penalty), countingFeasibility(penalty));
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    engine.step(pop, rng);
    CHECK(penalty.feasibilityCalls == 60);
    CHECK(penalty.fitnessCalls == 60);

    Counters none;
    GAEngine plain = makeEngine(smallConfig(InfeasibilityPolicy::None),
                                countingFitness(none), countingFeasibility(none));
    Population pop2 = plain.createInitialPopulation(randomBits(), rng);
    plain.step(pop2, rng);
    CHECK(none.feasibilityCalls == 0);
    CHECK(none.fitnessCalls == 60);
}

void testRejectPolicyRetriesInfeasibleOffspring() {
    mt19937 rng(2);
    GAConfig config = smallConfig(InfeasibilityPolicy::Reject);
    config.populationSize = 10;

    for (int attempts : {1, 3, 5}) {
        Counters counters;
        config.maxRejectAttempts = attempts;
        GAEngine engine(config, countingFitness(counters),
                        [&counters](const Chromosome&) {
                            ++counters.feasibilityCalls;
                            return false;
                        },
                        nullptr, nullptr, make_shared<TournamentSelection>(2),
                        make_shared<GenerationalReplacement>(1));
        Population pop = engine.createInitialPopulation(randomBits(), rng);
        engine.step(pop, rng);

        CHECK(counters.feasibilityCalls == 10 + 5 * attempts * 2);
        CHECK(pop.size() == 10);  
    }

    Counters ok;
    config.maxRejectAttempts = 5;
    GAEngine engine(config, countingFitness(ok),
                    [&ok](const Chromosome&) { ++ok.feasibilityCalls; return true; },
                    nullptr, nullptr, make_shared<TournamentSelection>(2),
                    make_shared<GenerationalReplacement>(1));
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    engine.step(pop, rng);
    CHECK(ok.feasibilityCalls == 20);
}

void testRejectPolicyIncreasesFeasibleOffspring() {
    auto feasibleShare = [](InfeasibilityPolicy policy) {
        int feasible = 0, total = 0;
        for (unsigned seed = 1; seed <= 20; ++seed) {
            mt19937 rng(seed);
            GAConfig config = smallConfig(policy);
            config.populationSize = 40;
            config.generations = 1;
            config.maxRejectAttempts = 10;
            Counters counters;
            GAEngine engine = makeEngine(config, countingFitness(counters),
                                         countingFeasibility(counters), 0);
            Population pop;
            for (int i = 0; i < config.populationSize; ++i) {
                BinaryChromosome c(vector<uint8_t>(kBits, 0));
                vector<int> idx(kBits);
                for (int k = 0; k < kBits; ++k) idx[k] = k;
                shuffle(idx.begin(), idx.end(), rng);
                for (int k = 0; k < kMaxOnes; ++k) c[idx[static_cast<int>(k)]] = 1;
                pop.addIndividual(make_unique<BinaryChromosome>(c));
            }
            engine.step(pop, rng);
            for (int i = 0; i < pop.size(); ++i) {
                feasible += onesIn(pop.getIndividual(i)) <= kMaxOnes;
                ++total;
            }
        }
        return static_cast<double>(feasible) / total;
    };

    const double withReject = feasibleShare(InfeasibilityPolicy::Reject);
    const double without = feasibleShare(InfeasibilityPolicy::None);
    CHECK(withReject > without);
    CHECK(withReject > 0.9);
}


void testPenaltyPolicyFindsConstrainedOptimum() {
    for (InfeasibilityPolicy policy :
         {InfeasibilityPolicy::Penalty, InfeasibilityPolicy::Reject}) {
        for (unsigned seed = 1; seed <= 5; ++seed) {
            mt19937 rng(seed);
            Counters counters;
            GAConfig config = smallConfig(policy);
            config.generations = 60;
            GAEngine engine = makeEngine(config, countingFitness(counters),
                                         countingFeasibility(counters));
            Population pop = engine.createInitialPopulation(randomBits(), rng);
            GAResult result = engine.run(pop, rng);

            CHECK(result.bestIsFeasible);
            CHECK(onesIn(*result.best) <= kMaxOnes);          
            CHECK(result.bestFitness == onesIn(*result.best)); 
            CHECK(result.bestFitness >= kMaxOnes - 1);         

            const double share = feasibleShare(pop);
            CHECK(share >= (policy == InfeasibilityPolicy::Reject ? 0.9 : 0.4));
        }
    }
}


void testPolicyKeepsPopulationFeasible() {
    auto meanShare = [](InfeasibilityPolicy policy) {
        double total = 0.0;
        const unsigned seeds = 8;
        for (unsigned seed = 1; seed <= seeds; ++seed) {
            mt19937 rng(seed);
            Counters counters;
            GAConfig config = smallConfig(policy);
            config.generations = 60;
            GAEngine engine = makeEngine(config, countingFitness(counters),
                                         countingFeasibility(counters));
            Population pop = engine.createInitialPopulation(randomBits(), rng);
            engine.run(pop, rng);
            total += feasibleShare(pop);
        }
        return total / seeds;
    };

    const double none = meanShare(InfeasibilityPolicy::None);
    const double penalty = meanShare(InfeasibilityPolicy::Penalty);
    const double reject = meanShare(InfeasibilityPolicy::Reject);

    CHECK(none < 0.2);                 
    CHECK(penalty > none + 0.4);       
    CHECK(reject > 0.9);               
}

void testNonePolicyStillReturnsBestFeasible() {
    
   
    mt19937 rng(7);
    Counters counters;
    GAConfig config = smallConfig(InfeasibilityPolicy::None);
    config.generations = 60;
    GAEngine engine = makeEngine(config, countingFitness(counters),
                                 countingFeasibility(counters));
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    GAResult result = engine.run(pop, rng);

    CHECK(result.bestIsFeasible);
    CHECK(onesIn(*result.best) <= kMaxOnes);
}

void testRunNeverLosesBestFeasibleEvenWithoutElitism() {

    for (unsigned seed = 1; seed <= 5; ++seed) {
        mt19937 rng(seed);
        double bestFeasibleSeen = -1.0;
        FitnessFunction fit = [](const Chromosome& c) {
            return static_cast<double>(onesIn(c));
        };
        FeasibilityFunction feas = [&bestFeasibleSeen](const Chromosome& c) {
            const int ones = onesIn(c);
            if (ones <= kMaxOnes) {
                bestFeasibleSeen = max(bestFeasibleSeen, static_cast<double>(ones));
            }
            return ones <= kMaxOnes;
        };

        GAConfig config = smallConfig(InfeasibilityPolicy::Penalty);
        GAEngine engine = makeEngine(config, fit, feas, /*elitism=*/0);
        Population pop = engine.createInitialPopulation(randomBits(), rng);
        GAResult result = engine.run(pop, rng);

        CHECK(result.bestIsFeasible);
        CHECK(result.bestFitness == bestFeasibleSeen);
    }
}

void testRunWithoutFeasibilityFunction() {
    mt19937 rng(3);
    Counters counters;
    GAEngine engine = makeEngine(smallConfig(), countingFitness(counters), nullptr);
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    GAResult result = engine.run(pop, rng);

    
    CHECK(result.bestIsFeasible);
    CHECK(result.bestFitness >= 18);
    CHECK(result.bestFitness == onesIn(*result.best));
}

void testRunReportsInfeasibleWhenNothingFeasibleExists() {
    mt19937 rng(4);
    Counters counters;
    GAEngine engine = makeEngine(smallConfig(), countingFitness(counters),
                                 [](const Chromosome&) { return false; });
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    GAResult result = engine.run(pop, rng);

    CHECK(!result.bestIsFeasible);      
        CHECK(result.best != nullptr);      
}


void testRunHistoryCallbackAndCounts() {
    mt19937 rng(5);
    Counters counters;
    GAConfig config = smallConfig();
    config.generations = 12;
    GAEngine engine = makeEngine(config, countingFitness(counters),
                                 countingFeasibility(counters));
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    CHECK(static_cast<int>(pop.size()) == config.populationSize);

    vector<int> generations;
    vector<pair<bool, double>> trace;
    GAResult result = engine.run(
        pop, rng,
        [&](int generation, const Chromosome& best, double fitness, bool feasible) {
            generations.push_back(generation);
            trace.emplace_back(feasible, fitness);
            CHECK(fitness == onesIn(best));
            CHECK(feasible == (onesIn(best) <= kMaxOnes));
        });

    CHECK(result.generationsRun == 12);
    CHECK(result.bestFitnessHistory.size() == 13);
    CHECK(generations.size() == 13);
    for (int g = 0; g <= 12; ++g) {
        CHECK(generations[static_cast<int>(g)] == g);
        CHECK(result.bestFitnessHistory[static_cast<int>(g)] ==
              trace[static_cast<int>(g)].second);
    }

    for (int g = 1; g < trace.size(); ++g) {
        CHECK(trace[g] >= trace[g - 1]);
    }
    CHECK(result.bestFitnessHistory.back() == result.bestFitness);
    CHECK(pop.size() == 30);
}

void testRunEvaluatesEachIndividualOnce() {
    mt19937 rng(6);
    Counters counters;
    GAConfig config = smallConfig();
    config.generations = 10;
    GAEngine engine = makeEngine(config, countingFitness(counters),
                                 countingFeasibility(counters));
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    engine.run(pop, rng);

    CHECK(counters.fitnessCalls == 30 + 10 * 30);
    CHECK(counters.feasibilityCalls == 30 + 10 * 30);
}

void testRunIsReproducibleForTheSameSeed() {
    auto runOnce = [](unsigned seed) {
        mt19937 rng(seed);
        Counters counters;
        GAEngine engine = makeEngine(smallConfig(), countingFitness(counters),
                                     countingFeasibility(counters));
        Population pop = engine.createInitialPopulation(randomBits(), rng);
        GAResult result = engine.run(pop, rng);
        return make_pair(
            dynamic_cast<const BinaryChromosome&>(*result.best).genes(),
            result.bestFitnessHistory);
    };

    CHECK(runOnce(11) == runOnce(11));
}

void testRunKeepsEveryIndividualValid() {
    mt19937 rng(8);
    Counters counters;
    GAEngine engine = makeEngine(smallConfig(), countingFitness(counters),
                                 countingFeasibility(counters));
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    for (int g = 0; g < 15; ++g) {
        engine.step(pop, rng);
        CHECK(pop.size() == 30);
        for (int i = 0; i < pop.size(); ++i) {
            const auto* bits = dynamic_cast<const BinaryChromosome*>(&pop.getIndividual(i));
            CHECK(bits != nullptr && bits->isValid() && bits->size() == kBits);
        }
    }
}

void testStepWorksWithoutCrossoverAndMutation() {
    mt19937 rng(9);
    Counters counters;
    GAEngine engine(smallConfig(), countingFitness(counters),
                    countingFeasibility(counters), nullptr, nullptr,
                    make_shared<RouletteWheelSelection>(),
                    make_shared<SteadyStateReplacement>());
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    for (int g = 0; g < 5; ++g) {
        engine.step(pop, rng);
        CHECK(pop.size() == 30);
    }
}

void testOddPopulationSize() {
    mt19937 rng(10);
    Counters counters;
    GAConfig config = smallConfig();
    config.populationSize = 7;
    config.generations = 5;
    GAEngine engine = makeEngine(config, countingFitness(counters),
                                 countingFeasibility(counters));
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    GAResult result = engine.run(pop, rng);
    CHECK(pop.size() == 7);
    CHECK(result.best != nullptr);
}


void testStepAndRunValidateTheirSetup() {
    mt19937 rng(12);
    Counters counters;

    GAEngine bare(smallConfig(), countingFitness(counters), nullptr);
    Population pop = bare.createInitialPopulation(randomBits(), rng);
    CHECK_THROWS(bare.step(pop, rng), invalid_argument);
    CHECK_THROWS(bare.run(pop, rng), invalid_argument);

    bare.setSelection(make_shared<TournamentSelection>(2));
    CHECK_THROWS(bare.step(pop, rng), invalid_argument);  
    bare.setReplacement(make_shared<GenerationalReplacement>(1));
    bare.step(pop, rng);  

    GAEngine engine = makeEngine(smallConfig(), countingFitness(counters),
                                 countingFeasibility(counters));

   
    Population empty;
    CHECK_THROWS(engine.step(empty, rng), invalid_argument);
    CHECK_THROWS(engine.run(empty, rng), invalid_argument);

    
    Population wrongSize;
    for (int i = 0; i < 5; ++i) {
        wrongSize.addIndividual(make_unique<BinaryChromosome>(
            BinaryChromosome::random(kBits, rng)));
    }
    CHECK_THROWS(engine.run(wrongSize, rng), invalid_argument);
    engine.step(wrongSize, rng);  
    CHECK(wrongSize.size() == 5);
}

void testNonFiniteFitnessIsRejected() {
    mt19937 rng(13);
    for (double bad : {numeric_limits<double>::quiet_NaN(),
                       numeric_limits<double>::infinity(),
                       -numeric_limits<double>::infinity()}) {
        GAEngine engine(smallConfig(), [bad](const Chromosome&) { return bad; },
                        nullptr, nullptr, nullptr,
                        make_shared<TournamentSelection>(2),
                        make_shared<GenerationalReplacement>(1));
        Population pop = engine.createInitialPopulation(randomBits(), rng);
        CHECK_THROWS(engine.step(pop, rng), invalid_argument);
        CHECK_THROWS(engine.run(pop, rng), invalid_argument);
    }
}

void testUserFunctionExceptionsPropagate() {
    mt19937 rng(14);
    GAEngine engine(smallConfig(), [](const Chromosome&) -> double { return 1.0; },
                    [](const Chromosome&) -> bool { throw runtime_error("boom"); },
                    nullptr, nullptr, make_shared<TournamentSelection>(2),
                    make_shared<GenerationalReplacement>(1));
    Population pop = engine.createInitialPopulation(randomBits(), rng);
    CHECK_THROWS(engine.step(pop, rng), runtime_error);
}

void testCreateInitialPopulation() {
    mt19937 rng(15);
    Counters counters;
    GAConfig config = smallConfig();
    config.populationSize = 17;
    GAEngine engine = makeEngine(config, countingFitness(counters), nullptr);

    Population pop = engine.createInitialPopulation(randomBits(), rng);
    CHECK(pop.size() == 17);

    CHECK_THROWS(engine.createInitialPopulation(nullptr, rng), invalid_argument);
    CHECK_THROWS(engine.createInitialPopulation(
                     [](mt19937&) { return unique_ptr<Chromosome>(); }, rng),
                 invalid_argument);
}

void testEngineIsRepresentationAgnostic() {
   
    mt19937 rng(16);
    GAEngine engine(smallConfig(),
                    [](const Chromosome&) { return 1.0; }, nullptr,
                    nullptr, nullptr, make_shared<TournamentSelection>(2),
                    make_shared<GenerationalReplacement>(1));
    Population pop = engine.createInitialPopulation(
        [](mt19937&) { return make_unique<OtherChromosome>(); }, rng);
    engine.step(pop, rng);
    CHECK(pop.size() == 30);
}

int main() {
    RUN_TEST(testConfigDefaultsAndValidation);
    RUN_TEST(testConfigSetParameters);
    RUN_TEST(testEngineRequiresFitnessFunctionAndValidConfig);
    RUN_TEST(testUserFunctionsAreStoredAndCallable);
    RUN_TEST(testOperatorRatesMustMatchConfig);
    RUN_TEST(testPolicyNoneLeavesFitnessUntouched);
    RUN_TEST(testAdaptivePenaltyPutsInfeasibleBelowFeasible);
    RUN_TEST(testAdaptivePenaltyIsScaleFree);
    RUN_TEST(testAdaptivePenaltyEdgeCases);
    RUN_TEST(testFixedPenalty);
    RUN_TEST(testFeasibilityFunctionIsCalledByStep);
    RUN_TEST(testRejectPolicyRetriesInfeasibleOffspring);
    RUN_TEST(testRejectPolicyIncreasesFeasibleOffspring);
    RUN_TEST(testPenaltyPolicyFindsConstrainedOptimum);
    RUN_TEST(testPolicyKeepsPopulationFeasible);
    RUN_TEST(testNonePolicyStillReturnsBestFeasible);
    RUN_TEST(testRunNeverLosesBestFeasibleEvenWithoutElitism);
    RUN_TEST(testRunWithoutFeasibilityFunction);
    RUN_TEST(testRunReportsInfeasibleWhenNothingFeasibleExists);
    RUN_TEST(testRunHistoryCallbackAndCounts);
    RUN_TEST(testRunEvaluatesEachIndividualOnce);
    RUN_TEST(testRunIsReproducibleForTheSameSeed);
    RUN_TEST(testRunKeepsEveryIndividualValid);
    RUN_TEST(testStepWorksWithoutCrossoverAndMutation);
    RUN_TEST(testOddPopulationSize);
    RUN_TEST(testStepAndRunValidateTheirSetup);
    RUN_TEST(testNonFiniteFitnessIsRejected);
    RUN_TEST(testUserFunctionExceptionsPropagate);
    RUN_TEST(testCreateInitialPopulation);
    RUN_TEST(testEngineIsRepresentationAgnostic);

    return finishTests();
}
