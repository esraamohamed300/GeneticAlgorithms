#include "FloatChromosome.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
using namespace std;

FloatChromosome::FloatChromosome(
    vector<double> genes,
    double lower,
    double upper
)
    : GeneChromosome<double>(move(genes)),
      lower_(lower),
      upper_(upper)
{
    if (!isfinite(lower_) || !isfinite(upper_) || lower_ > upper_) {
        throw invalid_argument(
            "FloatChromosome: bounds must be finite with lower <= upper."
        );
    }
    if (!isValid()) {
        throw invalid_argument(
            "FloatChromosome: a gene is NaN, infinite or outside "
            "[lower, upper]."
        );
    }
}

FloatChromosome FloatChromosome::random(
    int length,
    double lower,
    double upper,
    mt19937& rng
) {
    if (!isfinite(lower) || !isfinite(upper) || lower > upper) {
        throw invalid_argument(
            "FloatChromosome::random: bounds must be finite with "
            "lower <= upper."
        );
    }

    vector<double> genes(length, lower);

    
    if (lower < upper) {
        uniform_real_distribution<double> dist(lower, upper);
        for (double& g : genes) {
            g = dist(rng);
        }
    }
    return FloatChromosome(move(genes), lower, upper);
}

void FloatChromosome::clamp() {
    for (double& g : genes_) {
        if (isnan(g)) {
            g = lower_;
        } else {
            g = min(max(g, lower_), upper_);
        }
    }
}

bool FloatChromosome::isValid() const {
    for (double g : genes_) {
        if (!isfinite(g) || g < lower_ || g > upper_) {
            return false;
        }
    }
    return true;
}

unique_ptr<Chromosome> FloatChromosome::clone() const {
    return make_unique<FloatChromosome>(*this);
}

string FloatChromosome::toString() const {
    string s;
    for (int i = 0; i < genes_.size(); ++i) {
        if (i > 0) {
            s.push_back(' ');
        }
        s += to_string(genes_[i]);
    }
    return s;
}
