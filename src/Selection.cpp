#include "Selection.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// ---------------------------------------------------------------------------
// SelectionMethod
// ---------------------------------------------------------------------------

void SelectionMethod::validate(const std::vector<double>& fitness) {
    if (fitness.empty()) {
        throw std::invalid_argument(
            "Selection: the population has no individuals."
        );
    }
    for (double f : fitness) {
        if (!std::isfinite(f)) {
            throw std::invalid_argument(
                "Selection: fitness values must be finite numbers."
            );
        }
    }
}

std::vector<std::size_t> SelectionMethod::selectMany(
    const std::vector<double>& fitness,
    std::size_t count,
    std::mt19937& rng
) const {
    std::vector<std::size_t> picked;
    picked.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        picked.push_back(select(fitness, rng));
    }
    return picked;
}

// ---------------------------------------------------------------------------
// Tournament
// ---------------------------------------------------------------------------

TournamentSelection::TournamentSelection(std::size_t tournamentSize)
    : tournamentSize_(tournamentSize)
{
    if (tournamentSize_ < 1) {
        throw std::invalid_argument(
            "TournamentSelection: tournament size must be at least 1."
        );
    }
}

std::size_t TournamentSelection::select(
    const std::vector<double>& fitness,
    std::mt19937& rng
) const {
    validate(fitness);

    std::uniform_int_distribution<std::size_t> pick(0, fitness.size() - 1);

    std::size_t best = pick(rng);
    for (std::size_t i = 1; i < tournamentSize_; ++i) {
        const std::size_t challenger = pick(rng);
        if (fitness[challenger] > fitness[best]) {
            best = challenger;  // ties keep the earlier contestant
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// Roulette wheel
// ---------------------------------------------------------------------------

namespace {

// Cumulative weights after shifting negatives up; empty result means
// "all weights are zero, use uniform selection".
std::vector<double> buildWheel(const std::vector<double>& fitness) {
    const double lowest = *std::min_element(fitness.begin(), fitness.end());
    const double shift = lowest < 0.0 ? -lowest : 0.0;

    std::vector<double> cumulative(fitness.size());
    double total = 0.0;
    for (std::size_t i = 0; i < fitness.size(); ++i) {
        total += fitness[i] + shift;
        cumulative[i] = total;
    }
    if (!(total > 0.0)) {
        cumulative.clear();
    }
    return cumulative;
}

std::size_t spin(
    const std::vector<double>& cumulative,
    std::size_t populationSize,
    std::mt19937& rng
) {
    if (cumulative.empty()) {
        std::uniform_int_distribution<std::size_t> any(0, populationSize - 1);
        return any(rng);
    }

    std::uniform_real_distribution<double> dist(0.0, cumulative.back());
    const double r = dist(rng);

    // First slot whose cumulative weight is strictly above r, so slots with
    // zero weight (equal to their predecessor) can never be returned.
    std::size_t index = static_cast<std::size_t>(
        std::upper_bound(cumulative.begin(), cumulative.end(), r) -
        cumulative.begin()
    );

    // Guard against floating-point round-off at the very end of the wheel.
    if (index >= cumulative.size()) {
        index = cumulative.size() - 1;
    }
    while (index > 0 && cumulative[index] == cumulative[index - 1]) {
        --index;
    }
    return index;
}

}  // namespace

std::size_t RouletteWheelSelection::select(
    const std::vector<double>& fitness,
    std::mt19937& rng
) const {
    validate(fitness);
    return spin(buildWheel(fitness), fitness.size(), rng);
}

std::vector<std::size_t> RouletteWheelSelection::selectMany(
    const std::vector<double>& fitness,
    std::size_t count,
    std::mt19937& rng
) const {
    validate(fitness);
    const std::vector<double> wheel = buildWheel(fitness);

    std::vector<std::size_t> picked;
    picked.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        picked.push_back(spin(wheel, fitness.size(), rng));
    }
    return picked;
}
