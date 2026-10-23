#pragma once

#include <cstddef>
#include <random>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// Parent selection
// ----------------------------------------------------------------------------
// FITNESS CONVENTION (shared with the engine and the CVRP runner):
//     HIGHER fitness = BETTER individual.
// Problems that minimise a cost (e.g. CVRP distance) must convert it before
// selection, e.g. fitness = 1 / (1 + cost). Selection only sees one double per
// individual, so it is independent of the chromosome representation.
//
// Selection methods return INDICES into the fitness vector (= indices into the
// population). The same individual may be returned more than once.
// All methods throw std::invalid_argument for an empty fitness vector or a
// NaN/infinite fitness value.
class SelectionMethod {
public:
    virtual ~SelectionMethod() = default;

    // Picks one parent index.
    virtual std::size_t select(const std::vector<double>& fitness,
                               std::mt19937& rng) const = 0;

    // Picks `count` parent indices (with repetition).
    virtual std::vector<std::size_t> selectMany(
        const std::vector<double>& fitness, std::size_t count,
        std::mt19937& rng) const;

    virtual std::string name() const = 0;

protected:
    static void validate(const std::vector<double>& fitness);
};

// Tournament selection: draw `tournamentSize` individuals uniformly at random
// (with replacement) and return the fittest of them. Larger tournaments mean
// stronger selection pressure; size 1 is uniform random selection.
// Works with negative fitness values.
class TournamentSelection : public SelectionMethod {
public:
    // Default size 3. Throws std::invalid_argument if tournamentSize < 1.
    explicit TournamentSelection(std::size_t tournamentSize = 3);

    std::size_t select(const std::vector<double>& fitness,
                       std::mt19937& rng) const override;

    std::size_t tournamentSize() const { return tournamentSize_; }
    std::string name() const override { return "Tournament"; }

private:
    std::size_t tournamentSize_;
};

// Roulette-wheel (fitness-proportionate) selection: individual i is chosen
// with probability fitness[i] / sum(fitness).
// If some fitness values are negative, all values are shifted up by
// -min(fitness) so the smallest becomes 0 (it then is never chosen).
// If every weight ends up 0 (all fitness equal and <= 0), selection is uniform.
class RouletteWheelSelection : public SelectionMethod {
public:
    std::size_t select(const std::vector<double>& fitness,
                       std::mt19937& rng) const override;

    // Builds the cumulative wheel once instead of once per pick.
    std::vector<std::size_t> selectMany(
        const std::vector<double>& fitness, std::size_t count,
        std::mt19937& rng) const override;

    std::string name() const override { return "RouletteWheel"; }
};
