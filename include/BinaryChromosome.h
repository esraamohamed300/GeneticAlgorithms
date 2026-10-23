#pragma once

#include "GeneChromosome.h"

#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

// Binary representation.
//
// Valid values : every gene is 0 or 1 (stored as std::uint8_t, not
//                std::vector<bool>, so genes can be referenced and swapped
//                like any other representation).
// Length       : any length >= 0, fixed by whoever creates the chromosome.
//
// Groups of bits can be read/written as unsigned integers (most significant
// bit first), which is how the CVRP codec stores one "sort key" per customer.
class BinaryChromosome : public GeneChromosome<std::uint8_t> {
public:
    BinaryChromosome() = default;

    // Throws std::invalid_argument if any gene is not 0 or 1.
    explicit BinaryChromosome(std::vector<std::uint8_t> bits);

    // Uniformly random bits.
    static BinaryChromosome random(std::size_t length, std::mt19937& rng);

    // Reads `bits` bits starting at `start` as an unsigned integer (MSB first).
    // `bits` must be in [1, 32]; throws std::out_of_range / invalid_argument.
    std::uint32_t decodeUnsigned(std::size_t start, std::size_t bits) const;

    // Writes `value` into `bits` bits starting at `start` (MSB first).
    // Throws if `value` does not fit into `bits` bits.
    void encodeUnsigned(std::size_t start, std::size_t bits,
                        std::uint32_t value);

    // Flips one gene (0 <-> 1). Throws std::out_of_range.
    void flip(std::size_t index);

    bool isValid() const override;

    std::unique_ptr<Chromosome> clone() const override;

    // e.g. "10110". Handy for debugging and tests.
    std::string toString() const;
};
