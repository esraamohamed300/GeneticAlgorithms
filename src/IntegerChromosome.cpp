#include "IntegerChromosome.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>

IntegerChromosome::IntegerChromosome(
    std::vector<int> genes,
    int lower,
    int upper,
    bool permutation
)
    : GeneChromosome<int>(std::move(genes)),
      lower_(lower),
      upper_(upper),
      permutation_(permutation)
{
    if (lower_ > upper_) {
        throw std::invalid_argument(
            "IntegerChromosome: lower bound is greater than upper bound."
        );
    }
    if (!isValid()) {
        throw std::invalid_argument(
            permutation_
                ? "IntegerChromosome: genes are not a permutation of "
                  "[lower, upper]."
                : "IntegerChromosome: a gene is outside [lower, upper]."
        );
    }
}

IntegerChromosome IntegerChromosome::random(
    std::size_t length,
    int lower,
    int upper,
    std::mt19937& rng
) {
    if (lower > upper) {
        throw std::invalid_argument(
            "IntegerChromosome::random: lower bound is greater than upper bound."
        );
    }

    std::uniform_int_distribution<int> dist(lower, upper);

    std::vector<int> genes(length);
    for (int& g : genes) {
        g = dist(rng);
    }
    return IntegerChromosome(std::move(genes), lower, upper, false);
}

IntegerChromosome IntegerChromosome::randomPermutation(
    int lower,
    int upper,
    std::mt19937& rng
) {
    if (lower > upper) {
        throw std::invalid_argument(
            "IntegerChromosome::randomPermutation: lower bound is greater "
            "than upper bound."
        );
    }

    std::vector<int> genes(static_cast<std::size_t>(
        static_cast<long long>(upper) - lower + 1
    ));
    std::iota(genes.begin(), genes.end(), lower);

    // Fisher-Yates shuffle, written by hand so results depend only on the
    // generator and not on the standard library's std::shuffle algorithm.
    for (std::size_t i = genes.size(); i > 1; --i) {
        std::uniform_int_distribution<std::size_t> pick(0, i - 1);
        std::swap(genes[i - 1], genes[pick(rng)]);
    }
    return IntegerChromosome(std::move(genes), lower, upper, true);
}

bool IntegerChromosome::isValid() const {
    for (int g : genes_) {
        if (g < lower_ || g > upper_) {
            return false;
        }
    }

    if (!permutation_) {
        return true;
    }

    const long long range = static_cast<long long>(upper_) - lower_ + 1;
    if (static_cast<long long>(genes_.size()) != range) {
        return false;
    }

    std::vector<bool> seen(genes_.size(), false);
    for (int g : genes_) {
        const std::size_t slot = static_cast<std::size_t>(g - lower_);
        if (seen[slot]) {
            return false;
        }
        seen[slot] = true;
    }
    return true;
}

std::unique_ptr<Chromosome> IntegerChromosome::clone() const {
    return std::make_unique<IntegerChromosome>(*this);
}

std::string IntegerChromosome::toString() const {
    std::string s;
    for (std::size_t i = 0; i < genes_.size(); ++i) {
        if (i > 0) {
            s.push_back(' ');
        }
        s += std::to_string(genes_[i]);
    }
    return s;
}
