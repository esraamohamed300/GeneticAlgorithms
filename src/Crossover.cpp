#include "Crossover.h"

#include "BinaryChromosome.h"
#include "FloatChromosome.h"
#include "IntegerChromosome.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <vector>

using namespace std;

// Helper: picks `k` unique cut points from [1, maxVal] sorted in ascending order.
static vector<size_t> pickCutPoints(size_t k, size_t maxVal, mt19937& rng) {
    if (k == 0 || maxVal == 0) {
        return {};
    }

    size_t actualK = min(k, maxVal);
    unordered_set<size_t> uniqueCuts;
    uniform_int_distribution<size_t> dist(1, maxVal);

    while (uniqueCuts.size() < actualK) {
        uniqueCuts.insert(dist(rng));
    }

    vector<size_t> cuts(uniqueCuts.begin(), uniqueCuts.end());
    sort(cuts.begin(), cuts.end());
    return cuts;
}

// Helper: swaps alternating segments between two gene vectors based on cut points.
template <typename T>
static void swapAlternatingSegments(vector<T>& g1, vector<T>& g2, const vector<size_t>& cuts) {
    bool doSwap = false;
    size_t start = 0;

    for (size_t cut : cuts) {
        if (doSwap) {
            for (size_t i = start; i < cut; ++i) {
                swap(g1[i], g2[i]);
            }
        }
        doSwap = !doSwap;
        start = cut;
    }

    // Last segment from the last cut point to the end of the chromosome
    if (doSwap) {
        for (size_t i = start; i < g1.size(); ++i) {
            swap(g1[i], g2[i]);
        }
    }
}

// Helper: builds one child for Order Crossover (OX).
static IntegerChromosome buildOXChild(const IntegerChromosome& parentFrom,
                                      const IntegerChromosome& parentOther,
                                      size_t cut1,
                                      size_t cut2) {
    size_t n = parentFrom.size();
    int lower = parentFrom.lowerBound();
    int upper = parentFrom.upperBound();

    vector<int> childGenes(n);
    vector<bool> inChild(upper - lower + 1, false);

    // 1. Copy segment [cut1, cut2] directly from parentFrom
    for (size_t i = cut1; i <= cut2; ++i) {
        int gene = parentFrom[i];
        childGenes[i] = gene;
        inChild[gene - lower] = true;
    }

    // 2. Fill remaining positions circularly starting immediately after cut2
    size_t writeIdx = (cut2 + 1) % n;
    size_t readIdx = (cut2 + 1) % n;

    for (size_t count = 0; count < n; ++count) {
        int candidate = parentOther[readIdx];
        readIdx = (readIdx + 1) % n;

        if (!inChild[candidate - lower]) {
            childGenes[writeIdx] = candidate;
            inChild[candidate - lower] = true;
            writeIdx = (writeIdx + 1) % n;
        }
    }

    return IntegerChromosome(childGenes, lower, upper, true);
}

// ----------------------------------------------------------------------------
// MultiPointCrossover
// ----------------------------------------------------------------------------

MultiPointCrossover::MultiPointCrossover(size_t numPoints, double crossoverRate)
    : numPoints_(numPoints), crossoverRate_(crossoverRate) {
    if (numPoints_ < 1) {
        throw invalid_argument("MultiPointCrossover: numPoints must be >= 1.");
    }
    if (crossoverRate_ < 0.0 || crossoverRate_ > 1.0) {
        throw invalid_argument("MultiPointCrossover: crossoverRate must be in [0.0, 1.0].");
    }
}

pair<unique_ptr<Chromosome>, unique_ptr<Chromosome>>
MultiPointCrossover::cross(const Chromosome& parent1, const Chromosome& parent2,
                           mt19937& rng) const {
    // If crossover probability test fails, return exact clones of parents
    uniform_real_distribution<double> dist01(0.0, 1.0);
    if (crossoverRate_ <= 0.0 || dist01(rng) >= crossoverRate_) {
        return {parent1.clone(), parent2.clone()};
    }

    // BinaryChromosome
    if (const auto* b1 = dynamic_cast<const BinaryChromosome*>(&parent1)) {
        const auto* b2 = dynamic_cast<const BinaryChromosome*>(&parent2);
        if (!b2) {
            throw invalid_argument("MultiPointCrossover: parent types do not match (parent1 is Binary).");
        }
        if (b1->size() != b2->size()) {
            throw invalid_argument("MultiPointCrossover: parents have different lengths.");
        }
        auto c1 = b1->clone();
        auto c2 = b2->clone();
        if (b1->size() > 1) {
            vector<size_t> cuts = pickCutPoints(numPoints_, b1->size() - 1, rng);
            auto* typed1 = static_cast<BinaryChromosome*>(c1.get());
            auto* typed2 = static_cast<BinaryChromosome*>(c2.get());
            swapAlternatingSegments(typed1->genes(), typed2->genes(), cuts);
        }
        return {move(c1), move(c2)};
    }

    // FloatChromosome
    if (const auto* f1 = dynamic_cast<const FloatChromosome*>(&parent1)) {
        const auto* f2 = dynamic_cast<const FloatChromosome*>(&parent2);
        if (!f2) {
            throw invalid_argument("MultiPointCrossover: parent types do not match (parent1 is Float).");
        }
        if (f1->size() != f2->size()) {
            throw invalid_argument("MultiPointCrossover: parents have different lengths.");
        }
        if (f1->lowerBound() != f2->lowerBound() || f1->upperBound() != f2->upperBound()) {
            throw invalid_argument("MultiPointCrossover: Float parents have different bounds.");
        }
        auto c1 = f1->clone();
        auto c2 = f2->clone();
        if (f1->size() > 1) {
            vector<size_t> cuts = pickCutPoints(numPoints_, f1->size() - 1, rng);
            auto* typed1 = static_cast<FloatChromosome*>(c1.get());
            auto* typed2 = static_cast<FloatChromosome*>(c2.get());
            swapAlternatingSegments(typed1->genes(), typed2->genes(), cuts);
        }
        return {move(c1), move(c2)};
    }

    // IntegerChromosome (free integers only)
    if (const auto* i1 = dynamic_cast<const IntegerChromosome*>(&parent1)) {
        const auto* i2 = dynamic_cast<const IntegerChromosome*>(&parent2);
        if (!i2) {
            throw invalid_argument("MultiPointCrossover: parent types do not match (parent1 is Integer).");
        }
        if (i1->size() != i2->size()) {
            throw invalid_argument("MultiPointCrossover: parents have different lengths.");
        }
        if (i1->isPermutation() || i2->isPermutation()) {
            throw invalid_argument(
                "MultiPointCrossover: cannot be applied to permutation chromosomes because "
                "segment swapping produces duplicates and missing genes. Use OrderCrossover instead."
            );
        }
        if (i1->lowerBound() != i2->lowerBound() || i1->upperBound() != i2->upperBound()) {
            throw invalid_argument("MultiPointCrossover: Integer parents have different bounds.");
        }
        auto c1 = i1->clone();
        auto c2 = i2->clone();
        if (i1->size() > 1) {
            vector<size_t> cuts = pickCutPoints(numPoints_, i1->size() - 1, rng);
            auto* typed1 = static_cast<IntegerChromosome*>(c1.get());
            auto* typed2 = static_cast<IntegerChromosome*>(c2.get());
            swapAlternatingSegments(typed1->genes(), typed2->genes(), cuts);
        }
        return {move(c1), move(c2)};
    }

    throw invalid_argument("MultiPointCrossover: unsupported chromosome type.");
}

// ----------------------------------------------------------------------------
// OrderCrossover (OX)
// ----------------------------------------------------------------------------

OrderCrossover::OrderCrossover(double crossoverRate)
    : crossoverRate_(crossoverRate) {
    if (crossoverRate_ < 0.0 || crossoverRate_ > 1.0) {
        throw invalid_argument("OrderCrossover: crossoverRate must be in [0.0, 1.0].");
    }
}

pair<unique_ptr<Chromosome>, unique_ptr<Chromosome>>
OrderCrossover::cross(const Chromosome& parent1, const Chromosome& parent2,
                      mt19937& rng) const {
    const auto* p1 = dynamic_cast<const IntegerChromosome*>(&parent1);
    const auto* p2 = dynamic_cast<const IntegerChromosome*>(&parent2);

    if (!p1 || !p2) {
        throw invalid_argument("OrderCrossover: both parents must be IntegerChromosome.");
    }
    if (!p1->isPermutation() || !p2->isPermutation()) {
        throw invalid_argument("OrderCrossover: both parents must have isPermutation() == true.");
    }
    if (p1->size() != p2->size()) {
        throw invalid_argument("OrderCrossover: parents have different lengths.");
    }
    if (p1->lowerBound() != p2->lowerBound() || p1->upperBound() != p2->upperBound()) {
        throw invalid_argument("OrderCrossover: parents have different bounds.");
    }
    if (!p1->isValid() || !p2->isValid()) {
        throw invalid_argument("OrderCrossover: parent permutation is invalid.");
    }

    // Skip crossover if probability is not met
    uniform_real_distribution<double> dist01(0.0, 1.0);
    if (crossoverRate_ <= 0.0 || dist01(rng) >= crossoverRate_) {
        return {p1->clone(), p2->clone()};
    }

    size_t n = p1->size();
    if (n <= 1) {
        return {p1->clone(), p2->clone()};
    }

    // Choose two cut points cut1 <= cut2
    uniform_int_distribution<size_t> distIdx(0, n - 1);
    size_t idxA = distIdx(rng);
    size_t idxB = distIdx(rng);
    size_t cut1 = min(idxA, idxB);
    size_t cut2 = max(idxA, idxB);

    auto child1 = make_unique<IntegerChromosome>(buildOXChild(*p1, *p2, cut1, cut2));
    auto child2 = make_unique<IntegerChromosome>(buildOXChild(*p2, *p1, cut1, cut2));

    return {move(child1), move(child2)};
}
