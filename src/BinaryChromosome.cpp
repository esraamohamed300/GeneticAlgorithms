#include "BinaryChromosome.h"

#include <stdexcept>
#include <utility>

BinaryChromosome::BinaryChromosome(std::vector<std::uint8_t> bits)
    : GeneChromosome<std::uint8_t>(std::move(bits))
{
    if (!isValid()) {
        throw std::invalid_argument(
            "BinaryChromosome genes must all be 0 or 1."
        );
    }
}

BinaryChromosome BinaryChromosome::random(
    std::size_t length,
    std::mt19937& rng
) {
    // uniform_int_distribution does not accept uint8_t, so draw ints.
    std::uniform_int_distribution<int> bit(0, 1);

    std::vector<std::uint8_t> bits(length);
    for (auto& b : bits) {
        b = static_cast<std::uint8_t>(bit(rng));
    }
    return BinaryChromosome(std::move(bits));
}

std::uint32_t BinaryChromosome::decodeUnsigned(
    std::size_t start,
    std::size_t bits
) const {
    if (bits < 1 || bits > 32) {
        throw std::invalid_argument(
            "decodeUnsigned: bits must be between 1 and 32."
        );
    }
    if (start > genes_.size() || bits > genes_.size() - start) {
        throw std::out_of_range(
            "decodeUnsigned: bit range exceeds chromosome length."
        );
    }

    std::uint32_t value = 0;
    for (std::size_t k = 0; k < bits; ++k) {
        value = (value << 1) | static_cast<std::uint32_t>(genes_[start + k]);
    }
    return value;
}

void BinaryChromosome::encodeUnsigned(
    std::size_t start,
    std::size_t bits,
    std::uint32_t value
) {
    if (bits < 1 || bits > 32) {
        throw std::invalid_argument(
            "encodeUnsigned: bits must be between 1 and 32."
        );
    }
    if (start > genes_.size() || bits > genes_.size() - start) {
        throw std::out_of_range(
            "encodeUnsigned: bit range exceeds chromosome length."
        );
    }
    if (bits < 32 && value >= (std::uint32_t{1} << bits)) {
        throw std::invalid_argument(
            "encodeUnsigned: value does not fit in the given number of bits."
        );
    }

    for (std::size_t k = 0; k < bits; ++k) {
        genes_[start + k] =
            static_cast<std::uint8_t>((value >> (bits - 1 - k)) & 1u);
    }
}

void BinaryChromosome::flip(std::size_t index) {
    genes_.at(index) ^= 1;
}

bool BinaryChromosome::isValid() const {
    for (std::uint8_t g : genes_) {
        if (g > 1) {
            return false;
        }
    }
    return true;
}

std::unique_ptr<Chromosome> BinaryChromosome::clone() const {
    return std::make_unique<BinaryChromosome>(*this);
}

std::string BinaryChromosome::toString() const {
    std::string s;
    s.reserve(genes_.size());
    for (std::uint8_t g : genes_) {
        s.push_back(g ? '1' : '0');
    }
    return s;
}
