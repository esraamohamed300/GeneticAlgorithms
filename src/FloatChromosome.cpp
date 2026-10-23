#include "FloatChromosome.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

FloatChromosome::FloatChromosome(
    std::vector<double> genes,
    double lower,
    double upper
)
    : GeneChromosome<double>(std::move(genes)),
      lower_(lower),
      upper_(upper)
{
    if (!std::isfinite(lower_) || !std::isfinite(upper_) || lower_ > upper_) {
        throw std::invalid_argument(
            "FloatChromosome: bounds must be finite with lower <= upper."
        );
    }
    if (!isValid()) {
        throw std::invalid_argument(
            "FloatChromosome: a gene is NaN, infinite or outside "
            "[lower, upper]."
        );
    }
}

FloatChromosome FloatChromosome::random(
    std::size_t length,
    double lower,
    double upper,
    std::mt19937& rng
) {
    if (!std::isfinite(lower) || !std::isfinite(upper) || lower > upper) {
        throw std::invalid_argument(
            "FloatChromosome::random: bounds must be finite with "
            "lower <= upper."
        );
    }

    std::vector<double> genes(length, lower);

    // uniform_real_distribution requires a < b; a degenerate range just
    // keeps every gene at the single allowed value.
    if (lower < upper) {
        std::uniform_real_distribution<double> dist(lower, upper);
        for (double& g : genes) {
            g = dist(rng);
        }
    }
    return FloatChromosome(std::move(genes), lower, upper);
}

void FloatChromosome::clamp() {
    for (double& g : genes_) {
        if (std::isnan(g)) {
            g = lower_;
        } else {
            g = std::min(std::max(g, lower_), upper_);
        }
    }
}

bool FloatChromosome::isValid() const {
    for (double g : genes_) {
        if (!std::isfinite(g) || g < lower_ || g > upper_) {
            return false;
        }
    }
    return true;
}

std::unique_ptr<Chromosome> FloatChromosome::clone() const {
    return std::make_unique<FloatChromosome>(*this);
}

std::string FloatChromosome::toString() const {
    std::string s;
    for (std::size_t i = 0; i < genes_.size(); ++i) {
        if (i > 0) {
            s.push_back(' ');
        }
        s += std::to_string(genes_[i]);
    }
    return s;
}
