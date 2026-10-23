#include "BinaryChromosome.h"

#include <stdexcept>
#include <utility>
using namespace std;

BinaryChromosome::BinaryChromosome(vector<uint8_t> bits)
    : GeneChromosome<uint8_t>(move(bits))
{
    if (!isValid()) {
        throw invalid_argument(
            "BinaryChromosome genes must all be 0 or 1."
        );
    }
}

BinaryChromosome BinaryChromosome::random(
    int length,
    mt19937& rng
) {
    uniform_int_distribution<int> bit(0, 1);

    vector<uint8_t> bits(length);
    for (auto& b : bits) {
        b = static_cast<uint8_t>(bit(rng));
    }
    return BinaryChromosome(move(bits));
}

uint32_t BinaryChromosome::decodeUnsigned(
    int start,
    int bits
) const {
    if (bits < 1 || bits > 32) {
        throw invalid_argument(
            "decodeUnsigned: bits must be between 1 and 32."
        );
    }
    if (start > genes_.size() || bits > genes_.size() - start) {
        throw out_of_range(
            "decodeUnsigned: bit range exceeds chromosome length."
        );
    }

    uint32_t value = 0;
    for (int k = 0; k < bits; ++k) {
        value = (value << 1) | static_cast<uint32_t>(genes_[start + k]);
    }
    return value;
}

void BinaryChromosome::encodeUnsigned(
    int start,
    int bits,
    uint32_t value
) {
    if (bits < 1 || bits > 32) {
        throw invalid_argument(
            "encodeUnsigned: bits must be between 1 and 32."
        );
    }
    if (start > genes_.size() || bits > genes_.size() - start) {
        throw out_of_range(
            "encodeUnsigned: bit range exceeds chromosome length."
        );
    }
    if (bits < 32 && value >= (uint32_t{1} << bits)) {
        throw invalid_argument(
            "encodeUnsigned: value does not fit in the given number of bits."
        );
    }

    for (int k = 0; k < bits; ++k) {
        genes_[start + k] =
            static_cast<uint8_t>((value >> (bits - 1 - k)) & 1u);
    }
}

void BinaryChromosome::flip(int index) {
    genes_.at(index) ^= 1;
}

bool BinaryChromosome::isValid() const {
    for (uint8_t g : genes_) {
        if (g > 1) {
            return false;
        }
    }
    return true;
}

unique_ptr<Chromosome> BinaryChromosome::clone() const {
    return make_unique<BinaryChromosome>(*this);
}

string BinaryChromosome::toString() const {
    string s;
    s.reserve(genes_.size());
    for (
        uint8_t g : genes_) {
        s.push_back(g ? '1' : '0');
    }
    return s;
}
