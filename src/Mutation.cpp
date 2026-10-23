#include "Mutation.h"

#include "BinaryChromosome.h"
#include "FloatChromosome.h"
#include "IntegerChromosome.h"

#include <algorithm>
#include <stdexcept>

using namespace std;

// ----------------------------------------------------------------------------
// BitFlipMutation
// ----------------------------------------------------------------------------

BitFlipMutation::BitFlipMutation(double mutationRate)
    : mutationRate_(mutationRate) {
    if (mutationRate_ < 0.0 || mutationRate_ > 1.0) {
        throw invalid_argument("BitFlipMutation: mutationRate must be in [0.0, 1.0].");
    }
}

unique_ptr<Chromosome> BitFlipMutation::mutate(const Chromosome& individual, mt19937& rng) const {
    auto copy = individual.clone();
    mutateInPlace(*copy, rng);
    return copy;
}

void BitFlipMutation::mutateInPlace(Chromosome& individual, mt19937& rng) const {
    auto* binary = dynamic_cast<BinaryChromosome*>(&individual);
    if (!binary) {
        throw invalid_argument("BitFlipMutation: chromosome is not a BinaryChromosome.");
    }

    if (mutationRate_ <= 0.0 || binary->size() == 0) {
        return;
    }

    uniform_real_distribution<double> dist01(0.0, 1.0);
    size_t len = binary->size();
    for (size_t i = 0; i < len; ++i) {
        if (dist01(rng) < mutationRate_) {
            binary->flip(i);
        }
    }
}

// ----------------------------------------------------------------------------
// SwapMutation
// ----------------------------------------------------------------------------

SwapMutation::SwapMutation(double mutationRate)
    : mutationRate_(mutationRate) {
    if (mutationRate_ < 0.0 || mutationRate_ > 1.0) {
        throw invalid_argument("SwapMutation: mutationRate must be in [0.0, 1.0].");
    }
}

unique_ptr<Chromosome> SwapMutation::mutate(const Chromosome& individual, mt19937& rng) const {
    auto copy = individual.clone();
    mutateInPlace(*copy, rng);
    return copy;
}

void SwapMutation::mutateInPlace(Chromosome& individual, mt19937& rng) const {
    if (mutationRate_ <= 0.0) {
        return;
    }

    uniform_real_distribution<double> dist01(0.0, 1.0);
    if (dist01(rng) >= mutationRate_) {
        return;
    }

    auto* ic = dynamic_cast<IntegerChromosome*>(&individual);
    auto* fc = dynamic_cast<FloatChromosome*>(&individual);
    auto* bc = dynamic_cast<BinaryChromosome*>(&individual);

    if (!ic && !fc && !bc) {
        throw invalid_argument("SwapMutation: unsupported chromosome type.");
    }

    size_t len = ic ? ic->size() : (fc ? fc->size() : bc->size());
    if (len < 2) {
        return;  // cannot swap distinct positions on length 0 or 1
    }

    // Pick two distinct indices i and j
    uniform_int_distribution<size_t> dist(0, len - 1);
    size_t i = dist(rng);
    size_t j = dist(rng);
    while (j == i) {
        j = dist(rng);
    }

    if (ic) {
        swap((*ic)[i], (*ic)[j]);
    } else if (fc) {
        swap((*fc)[i], (*fc)[j]);
    } else {
        swap((*bc)[i], (*bc)[j]);
    }
}

// ----------------------------------------------------------------------------
// InversionMutation
// ----------------------------------------------------------------------------

InversionMutation::InversionMutation(double mutationRate)
    : mutationRate_(mutationRate) {
    if (mutationRate_ < 0.0 || mutationRate_ > 1.0) {
        throw invalid_argument("InversionMutation: mutationRate must be in [0.0, 1.0].");
    }
}

unique_ptr<Chromosome> InversionMutation::mutate(const Chromosome& individual, mt19937& rng) const {
    auto copy = individual.clone();
    mutateInPlace(*copy, rng);
    return copy;
}

void InversionMutation::mutateInPlace(Chromosome& individual, mt19937& rng) const {
    if (mutationRate_ <= 0.0) {
        return;
    }

    uniform_real_distribution<double> dist01(0.0, 1.0);
    if (dist01(rng) >= mutationRate_) {
        return;
    }

    auto* ic = dynamic_cast<IntegerChromosome*>(&individual);
    auto* fc = dynamic_cast<FloatChromosome*>(&individual);
    auto* bc = dynamic_cast<BinaryChromosome*>(&individual);

    if (!ic && !fc && !bc) {
        throw invalid_argument("InversionMutation: unsupported chromosome type.");
    }

    size_t len = ic ? ic->size() : (fc ? fc->size() : bc->size());
    if (len < 2) {
        return;  // cannot invert sub-segment of length 0 or 1
    }

    // Pick two cut points cut1 <= cut2
    uniform_int_distribution<size_t> dist(0, len - 1);
    size_t cut1 = dist(rng);
    size_t cut2 = dist(rng);
    if (cut1 > cut2) {
        swap(cut1, cut2);
    }

    if (ic) {
        reverse(ic->genes().begin() + cut1, ic->genes().begin() + cut2 + 1);
    } else if (fc) {
        reverse(fc->genes().begin() + cut1, fc->genes().begin() + cut2 + 1);
    } else {
        reverse(bc->genes().begin() + cut1, bc->genes().begin() + cut2 + 1);
    }
}
