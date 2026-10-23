#include "CVRPCodec.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>

namespace {

// ceil(log2(value)) for value >= 1.
int ceilLog2(int value) {
    int bits = 0;
    while ((1LL << bits) < value) {
        ++bits;
    }
    return bits;
}

// Indices 0..keys.size()-1 sorted by ascending key; ties keep index order.
// Returned as 1-based permutation values (index i  ->  value i+1).
template <typename Key>
std::vector<int> orderByKey(const std::vector<Key>& keys) {
    std::vector<int> order(keys.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return keys[static_cast<std::size_t>(a)] <
               keys[static_cast<std::size_t>(b)];
    });
    for (int& v : order) {
        ++v;
    }
    return order;
}

}  // namespace

CVRPCodec::CVRPCodec(int numCustomers, int numVehicles, int bitsPerKey)
    : numCustomers_(numCustomers),
      numVehicles_(numVehicles),
      bitsPerKey_(bitsPerKey)
{
    if (numCustomers_ < 1) {
        throw std::invalid_argument("CVRPCodec: need at least 1 customer.");
    }
    if (numVehicles_ < 1) {
        throw std::invalid_argument("CVRPCodec: need at least 1 vehicle.");
    }

    if (bitsPerKey_ == 0) {
        bitsPerKey_ = std::max(8, ceilLog2(permutationLength()));
    }
    if (bitsPerKey_ < 1 || bitsPerKey_ > 31) {
        throw std::invalid_argument(
            "CVRPCodec: bitsPerKey must be between 1 and 31."
        );
    }
}

std::size_t CVRPCodec::binaryLength() const {
    return static_cast<std::size_t>(permutationLength()) *
           static_cast<std::size_t>(bitsPerKey_);
}

// ---------------------------------------------------------------------------
// Creation
// ---------------------------------------------------------------------------

IntegerChromosome CVRPCodec::createInteger(std::mt19937& rng) const {
    return IntegerChromosome::randomPermutation(1, permutationLength(), rng);
}

FloatChromosome CVRPCodec::createFloat(std::mt19937& rng) const {
    return FloatChromosome::random(
        static_cast<std::size_t>(permutationLength()), 0.0, 1.0, rng
    );
}

BinaryChromosome CVRPCodec::createBinary(std::mt19937& rng) const {
    return BinaryChromosome::random(binaryLength(), rng);
}

// ---------------------------------------------------------------------------
// Decoding
// ---------------------------------------------------------------------------

Routes CVRPCodec::decodePermutation(const std::vector<int>& permutation) const {
    const int length = permutationLength();

    if (static_cast<int>(permutation.size()) != length) {
        throw std::invalid_argument(
            "CVRPCodec: giant tour must have length " +
            std::to_string(length) + ", got " +
            std::to_string(permutation.size()) + "."
        );
    }

    std::vector<bool> seen(static_cast<std::size_t>(length), false);
    for (int value : permutation) {
        if (value < 1 || value > length ||
            seen[static_cast<std::size_t>(value - 1)]) {
            throw std::invalid_argument(
                "CVRPCodec: giant tour is not a permutation of 1.." +
                std::to_string(length) + "."
            );
        }
        seen[static_cast<std::size_t>(value - 1)] = true;
    }

    Routes routes(1);
    for (int value : permutation) {
        if (isCustomer(value)) {
            routes.back().push_back(value);
        } else {
            routes.emplace_back();  // separator: next vehicle starts
        }
    }
    return routes;  // K-1 separators -> exactly K routes
}

Routes CVRPCodec::decode(const IntegerChromosome& chromosome) const {
    if (chromosome.lowerBound() != 1 ||
        chromosome.upperBound() != permutationLength()) {
        throw std::invalid_argument(
            "CVRPCodec: integer chromosome bounds must be [1, " +
            std::to_string(permutationLength()) + "]."
        );
    }
    return decodePermutation(chromosome.genes());
}

Routes CVRPCodec::decode(const FloatChromosome& chromosome) const {
    if (static_cast<int>(chromosome.size()) != permutationLength()) {
        throw std::invalid_argument(
            "CVRPCodec: float chromosome must have length " +
            std::to_string(permutationLength()) + "."
        );
    }
    if (!chromosome.isValid()) {
        throw std::invalid_argument(
            "CVRPCodec: float chromosome contains an invalid gene."
        );
    }
    return decodePermutation(permutationFromKeys(chromosome.genes()));
}

Routes CVRPCodec::decode(const BinaryChromosome& chromosome) const {
    if (chromosome.size() != binaryLength()) {
        throw std::invalid_argument(
            "CVRPCodec: binary chromosome must have length " +
            std::to_string(binaryLength()) + "."
        );
    }
    if (!chromosome.isValid()) {
        throw std::invalid_argument(
            "CVRPCodec: binary chromosome contains a gene that is not 0/1."
        );
    }

    const std::size_t bits = static_cast<std::size_t>(bitsPerKey_);
    std::vector<std::uint32_t> keys(
        static_cast<std::size_t>(permutationLength())
    );
    for (std::size_t i = 0; i < keys.size(); ++i) {
        keys[i] = chromosome.decodeUnsigned(i * bits, bits);
    }
    return decodePermutation(permutationFromKeys(keys));
}

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------

std::vector<int> CVRPCodec::routesToPermutation(const Routes& routes) const {
    if (static_cast<int>(routes.size()) > numVehicles_) {
        throw std::invalid_argument(
            "CVRPCodec: " + std::to_string(routes.size()) +
            " routes given but only " + std::to_string(numVehicles_) +
            " vehicles are available."
        );
    }

    std::vector<bool> served(static_cast<std::size_t>(numCustomers_), false);
    std::size_t servedCount = 0;
    for (const Route& route : routes) {
        for (int customer : route) {
            if (!isCustomer(customer)) {
                throw std::invalid_argument(
                    "CVRPCodec: invalid customer id " +
                    std::to_string(customer) + " (valid ids are 1.." +
                    std::to_string(numCustomers_) + ")."
                );
            }
            if (served[static_cast<std::size_t>(customer - 1)]) {
                throw std::invalid_argument(
                    "CVRPCodec: customer " + std::to_string(customer) +
                    " appears more than once."
                );
            }
            served[static_cast<std::size_t>(customer - 1)] = true;
            ++servedCount;
        }
    }
    if (static_cast<int>(servedCount) != numCustomers_) {
        throw std::invalid_argument(
            "CVRPCodec: " +
            std::to_string(numCustomers_ - static_cast<int>(servedCount)) +
            " customer(s) are not served by any route."
        );
    }

    std::vector<int> permutation;
    permutation.reserve(static_cast<std::size_t>(permutationLength()));
    for (int k = 0; k < numVehicles_; ++k) {
        if (k < static_cast<int>(routes.size())) {
            const Route& route = routes[static_cast<std::size_t>(k)];
            permutation.insert(permutation.end(), route.begin(), route.end());
        }
        if (k < numVehicles_ - 1) {
            permutation.push_back(numCustomers_ + 1 + k);  // separator
        }
    }
    return permutation;
}

IntegerChromosome CVRPCodec::encodeInteger(const Routes& routes) const {
    return IntegerChromosome(
        routesToPermutation(routes), 1, permutationLength(), true
    );
}

FloatChromosome CVRPCodec::encodeFloat(const Routes& routes) const {
    const std::vector<int> permutation = routesToPermutation(routes);
    const double length = static_cast<double>(permutation.size());

    // Value v sits at position p -> its key is (p + 0.5) / L, strictly inside
    // (0, 1) and strictly increasing with p, so sorting by key restores p.
    std::vector<double> keys(permutation.size());
    for (std::size_t p = 0; p < permutation.size(); ++p) {
        keys[static_cast<std::size_t>(permutation[p] - 1)] =
            (static_cast<double>(p) + 0.5) / length;
    }
    return FloatChromosome(std::move(keys), 0.0, 1.0);
}

BinaryChromosome CVRPCodec::encodeBinary(const Routes& routes) const {
    const std::vector<int> permutation = routesToPermutation(routes);

    if ((1LL << bitsPerKey_) < static_cast<long long>(permutation.size())) {
        throw std::invalid_argument(
            "CVRPCodec: bitsPerKey is too small to encode a full "
            "permutation without ties."
        );
    }

    // Value v at position p gets integer key p (unique, increasing with p).
    const std::size_t bits = static_cast<std::size_t>(bitsPerKey_);
    BinaryChromosome chromosome(
        std::vector<std::uint8_t>(binaryLength(), 0)
    );
    for (std::size_t p = 0; p < permutation.size(); ++p) {
        const std::size_t slot =
            static_cast<std::size_t>(permutation[p] - 1);
        chromosome.encodeUnsigned(
            slot * bits, bits, static_cast<std::uint32_t>(p)
        );
    }
    return chromosome;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool CVRPCodec::tryDecode(const Chromosome& chromosome, Routes& routes) const {
    try {
        if (auto* i = dynamic_cast<const IntegerChromosome*>(&chromosome)) {
            routes = decode(*i);
        } else if (auto* f = dynamic_cast<const FloatChromosome*>(&chromosome)) {
            routes = decode(*f);
        } else if (auto* b = dynamic_cast<const BinaryChromosome*>(&chromosome)) {
            routes = decode(*b);
        } else {
            return false;
        }
    } catch (const std::invalid_argument&) {
        return false;
    }
    return true;
}

Route CVRPCodec::withDepot(const Route& route) {
    Route full;
    full.reserve(route.size() + 2);
    full.push_back(0);
    full.insert(full.end(), route.begin(), route.end());
    full.push_back(0);
    return full;
}

std::vector<int> CVRPCodec::permutationFromKeys(
    const std::vector<double>& keys
) const {
    return orderByKey(keys);
}

std::vector<int> CVRPCodec::permutationFromKeys(
    const std::vector<std::uint32_t>& keys
) const {
    return orderByKey(keys);
}
