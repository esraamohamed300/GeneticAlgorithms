#pragma once

#include <memory>

using namespace std;        

class Chromosome {
public:
    virtual unique_ptr<Chromosome> clone() const = 0;

    virtual ~Chromosome() = default;
};
