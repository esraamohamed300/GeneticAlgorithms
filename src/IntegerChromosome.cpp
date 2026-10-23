#include "IntegerChromosome.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>
using namespace std;

IntegerChromosome::IntegerChromosome(
    vector<int> genes,
    int lower,
    int upper,
    bool permutation
)
    : GeneChromosome<int>(move(genes)),
      lower_(lower),
      upper_(upper),
      permutation_(permutation)
{
    if (lower_ > upper_) {
        throw invalid_argument(
            "IntegerChromosome: lower bound is greater than upper bound."
        );
    }
    if (!isValid()) {
        throw invalid_argument(
            permutation_
                ? "IntegerChromosome: genes are not a permutation of "
                  "[lower, upper]."
                : "IntegerChromosome: a gene is outside [lower, upper]."
        );
    }
}

IntegerChromosome IntegerChromosome::random(
    int length,
    int lower,
    int upper,
    mt19937& rng
) {
    if (lower > upper) {
        throw invalid_argument(
            "IntegerChromosome::random: lower bound is greater than upper bound."
        );
    }

    uniform_int_distribution<int> dist(lower, upper);

    vector<int> genes(length);
    for (int& g : genes) {
        g = dist(rng);
    }
    return IntegerChromosome(move(genes), lower, upper, false);
}

IntegerChromosome IntegerChromosome::randomPermutation(
    int lower,
    int upper,
    mt19937& rng
) {
    if (lower > upper) {
        throw invalid_argument(
            "IntegerChromosome::randomPermutation: lower bound is greater "
            "than upper bound."
        );
    }

    vector<int> genes(static_cast<int>(
        static_cast<long long>(upper) - lower + 1
    ));
    iota(genes.begin(), genes.end(), lower);

    
    for (int i = genes.size(); i > 1; --i) {
        uniform_int_distribution<int> pick(0, i - 1);
        swap(genes[i - 1], genes[pick(rng)]);
    }
    return IntegerChromosome(move(genes), lower, upper, true);
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

    vector<bool> seen(genes_.size(), false);
    for (int g : genes_) {
        const int slot = static_cast<int>(g - lower_);
        if (seen[slot]) {
            return false;
        }
        seen[slot] = true;
    }
    return true;
}

unique_ptr<Chromosome> IntegerChromosome::clone() const {
    return make_unique<IntegerChromosome>(*this);
}

string IntegerChromosome::toString() const {
    string s;
    for (int i = 0; i < genes_.size(); ++i) {
        if (i > 0) {
            s.push_back(' ');
        }
        s += to_string(genes_[i]);
    }
    return s;
}
