#include "Selection.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
using namespace std;


void SelectionMethod::validate(const vector<double>& fitness) {
    if (fitness.empty()) {
        throw invalid_argument(
            "Selection: the population has no individuals."
        );
    }
    for (double f : fitness) {
        if (!isfinite(f)) {
            throw invalid_argument(
                "Selection: fitness values must be finite numbers."
            );
        }
    }
}

vector<int> SelectionMethod::selectMany(
    const vector<double>& fitness,
    int count,
    mt19937& rng
) const {
    vector<int> picked;
    picked.reserve(count);
    for (int i = 0; i < count; ++i) {
        picked.push_back(select(fitness, rng));
    }
    return picked;
}



TournamentSelection::TournamentSelection(int tournamentSize)
    : tournamentSize_(tournamentSize)
{
    if (tournamentSize_ < 1) {
        throw invalid_argument(
            "TournamentSelection: tournament size must be at least 1."
        );
    }
}

int TournamentSelection::select(
    const vector<double>& fitness,
    mt19937& rng
) const {
    validate(fitness);

    uniform_int_distribution<int> pick(0, fitness.size() - 1);

    int best = pick(rng);
    for (int i = 1; i < tournamentSize_; ++i) {
        const int challenger = pick(rng);
        if (fitness[challenger] > fitness[best]) {
            best = challenger;  
        }
    }
    return best;
}





vector<double> buildWheel(const vector<double>& fitness) {
    const double lowest = *min_element(fitness.begin(), fitness.end());
    const double shift = lowest < 0.0 ? -lowest : 0.0;

    vector<double> cumulative(fitness.size());
    double total = 0.0;
    for (int i = 0; i < fitness.size(); ++i) {
        total += fitness[i] + shift;
        cumulative[i] = total;
    }
    if (!(total > 0.0)) {
        cumulative.clear();
    }
    return cumulative;
}

int spin(
    const vector<double>& cumulative,
    int populationSize,
    mt19937& rng
) {
    if (cumulative.empty()) {
        uniform_int_distribution<int> any(0, populationSize - 1);
        return any(rng);
    }

    uniform_real_distribution<double> dist(0.0, cumulative.back());
    const double r = dist(rng);

  
    int index = static_cast<int>(
        upper_bound(cumulative.begin(), cumulative.end(), r) -
        cumulative.begin()
    );

    
    if (index >= cumulative.size()) {
        index = cumulative.size() - 1;
    }
    while (index > 0 && cumulative[index] == cumulative[index - 1]) {
        --index;
    }
    return index;
}



int RouletteWheelSelection::select(
    const vector<double>& fitness,
    mt19937& rng
) const {
    validate(fitness);
    return spin(buildWheel(fitness), fitness.size(), rng);
}

vector<int> RouletteWheelSelection::selectMany(
    const vector<double>& fitness,
    int count,
    mt19937& rng
) const {
    validate(fitness);
    const vector<double> wheel = buildWheel(fitness);

    vector<int> picked;
    picked.reserve(count);
    for (int i = 0; i < count; ++i) {
        picked.push_back(spin(wheel, fitness.size(), rng));
    }
    return picked;
}
