#pragma once

#include "GeneChromosome.h"

#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace std;

class BinaryChromosome : public GeneChromosome<uint8_t> {
public:
    BinaryChromosome() = default;

    explicit BinaryChromosome(vector<uint8_t> bits);

    static BinaryChromosome random(int length, mt19937& rng);

    
    uint32_t decodeUnsigned(int start, int bits) const;


    void encodeUnsigned(int start, int bits,
                        uint32_t value);

    void flip(int index);

    bool isValid() const override;

    unique_ptr<Chromosome> clone() const override;

    string toString() const;
};
