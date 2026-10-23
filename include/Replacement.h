#pragma once

#include "Population.h"

#include <cstddef>
#include <string>
#include <vector>
using namespace std;


struct Survivor {
    bool fromOffspring;
    int index;
};

class ReplacementMethod {
public:
    virtual ~ReplacementMethod() = default;

    
    virtual vector<Survivor> chooseSurvivors(
        const vector<double>& parentFitness,
        const vector<double>& offspringFitness) const = 0;

   
    void replace(Population& population,
                 vector<double>& fitness,
                 const Population& offspring,
                 const vector<double>& offspringFitness) const;

    virtual string name() const = 0;

protected:
    static void validate(const vector<double>& parentFitness,
                         const vector<double>& offspringFitness);
};


class GenerationalReplacement : public ReplacementMethod {
public:
    explicit GenerationalReplacement(int elitism = 1);

    vector<Survivor> chooseSurvivors(
        const vector<double>& parentFitness,
        const vector<double>& offspringFitness) const override;

    int elitism() const { return elitism_; }
    string name() const override { return "Generational"; }

private:
    int elitism_;
};


class SteadyStateReplacement : public ReplacementMethod {
public:
    explicit SteadyStateReplacement(bool onlyIfBetter = true);

    vector<Survivor> chooseSurvivors(
        const vector<double>& parentFitness,
        const vector<double>& offspringFitness) const override;

    bool onlyIfBetter() const { return onlyIfBetter_; }
    string name() const override { return "SteadyState"; }

private:
    bool onlyIfBetter_;
};
