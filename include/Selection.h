#pragma once

#include <cstddef>
#include <random>
#include <string>
#include <vector>
using namespace std;


class SelectionMethod {
public:
    virtual ~SelectionMethod() = default;

    virtual int select(const vector<double>& fitness,
                               mt19937& rng) const = 0;

    virtual vector<int> selectMany(
        const vector<double>& fitness, int count,
        mt19937& rng) const;

    virtual string name() const = 0;

protected:
    static void validate(const vector<double>& fitness);
};


class TournamentSelection : public SelectionMethod {
public:
    explicit TournamentSelection(int tournamentSize = 3);

    int select(const vector<double>& fitness,
                       mt19937& rng) const override;

    int tournamentSize() const { return tournamentSize_; }
    string name() const override { return "Tournament"; }

private:
    int tournamentSize_;
};


class RouletteWheelSelection : public SelectionMethod {
public:
    int select(const vector<double>& fitness,
                       mt19937& rng) const override;

    vector<int> selectMany(
        const vector<double>& fitness, int count,
        mt19937& rng) const override;

    string name() const override { return "RouletteWheel"; }
};
