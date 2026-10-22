#pragma once

#include <memory>

class Chromosome {
public:
    virtual std::unique_ptr<Chromosome> clone() const = 0;

    virtual ~Chromosome() = default;
};
