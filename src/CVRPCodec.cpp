#include "CVRPCodec.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
using namespace std;

namespace {

int ceilLog2(int value) {
    int bits = 0;
    while ((1LL << bits) < value) {
        ++bits;
    }
    return bits;
}


template <typename Key>
vector<int> orderByKey(const vector<Key>& keys) {
    vector<int> order(keys.size());
    iota(order.begin(), order.end(), 0);
    stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return keys[static_cast<int>(a)] <
               keys[static_cast<int>(b)];
    });
    for (int& v : order) {
        ++v;
    }
    return order;
}

}  

CVRPCodec::CVRPCodec(int numCustomers, int numVehicles, int bitsPerKey)
    : numCustomers_(numCustomers),
      numVehicles_(numVehicles),
      bitsPerKey_(bitsPerKey)
{
    if (numCustomers_ < 1) {
        throw invalid_argument("CVRPCodec: need at least 1 customer.");
    }
    if (numVehicles_ < 1) {
        throw invalid_argument("CVRPCodec: need at least 1 vehicle.");
    }

    if (bitsPerKey_ == 0) {
        bitsPerKey_ = max(8, ceilLog2(permutationLength()));
    }
    if (bitsPerKey_ < 1 || bitsPerKey_ > 31) {
        throw invalid_argument(
            "CVRPCodec: bitsPerKey must be between 1 and 31."
        );
    }
}

int CVRPCodec::binaryLength() const {
    return static_cast<int>(permutationLength()) *
           static_cast<int>(bitsPerKey_);
}


IntegerChromosome CVRPCodec::createInteger(mt19937& rng) const {
    return IntegerChromosome::randomPermutation(1, permutationLength(), rng);
}

FloatChromosome CVRPCodec::createFloat(mt19937& rng) const {
    return FloatChromosome::random(
        static_cast<int>(permutationLength()), 0.0, 1.0, rng
    );
}

BinaryChromosome CVRPCodec::createBinary(mt19937& rng) const {
    return BinaryChromosome::random(binaryLength(), rng);
}



Routes CVRPCodec::decodePermutation(const vector<int>& permutation) const {
    const int length = permutationLength();

    if (static_cast<int>(permutation.size()) != length) {
        throw invalid_argument(
            "CVRPCodec: giant tour must have length " +
            to_string(length) + ", got " +
            to_string(permutation.size()) + "."
        );
    }

    vector<bool> seen(static_cast<int>(length), false);
    for (int value : permutation) {
        if (value < 1 || value > length ||
            seen[static_cast<int>(value - 1)]) {
            throw invalid_argument(
                "CVRPCodec: giant tour is not a permutation of 1.." +
                to_string(length) + "."
            );
        }
        seen[static_cast<int>(value - 1)] = true;
    }

    Routes routes(1);
    for (int value : permutation) {
        if (isCustomer(value)) {
            routes.back().push_back(value);
        } else {
            routes.emplace_back();  
        }
    }
    return routes;  
}

Routes CVRPCodec::decode(const IntegerChromosome& chromosome) const {
    if (chromosome.lowerBound() != 1 ||
        chromosome.upperBound() != permutationLength()) {
        throw invalid_argument(
            "CVRPCodec: integer chromosome bounds must be [1, " +
            to_string(permutationLength()) + "]."
        );
    }
    return decodePermutation(chromosome.genes());
}

Routes CVRPCodec::decode(const FloatChromosome& chromosome) const {
    if (static_cast<int>(chromosome.size()) != permutationLength()) {
        throw invalid_argument(
            "CVRPCodec: float chromosome must have length " +
            to_string(permutationLength()) + "."
        );
    }
    if (!chromosome.isValid()) {
        throw invalid_argument(
            "CVRPCodec: float chromosome contains an invalid gene."
        );
    }
    return decodePermutation(permutationFromKeys(chromosome.genes()));
}

Routes CVRPCodec::decode(const BinaryChromosome& chromosome) const {
    if (chromosome.size() != binaryLength()) {
        throw invalid_argument(
            "CVRPCodec: binary chromosome must have length " +
            to_string(binaryLength()) + "."
        );
    }
    if (!chromosome.isValid()) {
        throw invalid_argument(
            "CVRPCodec: binary chromosome contains a gene that is not 0/1."
        );
    }

    const int bits = static_cast<int>(bitsPerKey_);
    vector<uint32_t> keys(
        static_cast<int>(permutationLength())
    );
    for (int i = 0; i < keys.size(); ++i) {
        keys[i] = chromosome.decodeUnsigned(i * bits, bits);
    }
    return decodePermutation(permutationFromKeys(keys));
}



vector<int> CVRPCodec::routesToPermutation(const Routes& routes) const {
    if (static_cast<int>(routes.size()) > numVehicles_) {
        throw invalid_argument(
            "CVRPCodec: " + to_string(routes.size()) +
            " routes given but only " + to_string(numVehicles_) +
            " vehicles are available."
        );
    }

    vector<bool> served(static_cast<int>(numCustomers_), false);
    int servedCount = 0;
    for (const Route& route : routes) {
        for (int customer : route) {
            if (!isCustomer(customer)) {
                throw invalid_argument(
                    "CVRPCodec: invalid customer id " +
                    to_string(customer) + " (valid ids are 1.." +
                    to_string(numCustomers_) + ")."
                );
            }
            if (served[static_cast<int>(customer - 1)]) {
                throw invalid_argument(
                    "CVRPCodec: customer " + to_string(customer) +
                    " appears more than once."
                );
            }
            served[static_cast<int>(customer - 1)] = true;
            ++servedCount;
        }
    }
    if (static_cast<int>(servedCount) != numCustomers_) {
        throw invalid_argument(
            "CVRPCodec: " +
            to_string(numCustomers_ - static_cast<int>(servedCount)) +
            " customer(s) are not served by any route."
        );
    }

    vector<int> permutation;
    permutation.reserve(static_cast<int>(permutationLength()));
    for (int k = 0; k < numVehicles_; ++k) {
        if (k < static_cast<int>(routes.size())) {
            const Route& route = routes[static_cast<int>(k)];
            permutation.insert(permutation.end(), route.begin(), route.end());
        }
        if (k < numVehicles_ - 1) {
            permutation.push_back(numCustomers_ + 1 + k);  
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
    const vector<int> permutation = routesToPermutation(routes);
    const double length = static_cast<double>(permutation.size());

    
    vector<double> keys(permutation.size());
    for (int p = 0; p < permutation.size(); ++p) {
        keys[static_cast<int>(permutation[p] - 1)] =
            (static_cast<double>(p) + 0.5) / length;
    }
    return FloatChromosome(move(keys), 0.0, 1.0);
}

BinaryChromosome CVRPCodec::encodeBinary(const Routes& routes) const {
    const vector<int> permutation = routesToPermutation(routes);

    if ((1LL << bitsPerKey_) < static_cast<long long>(permutation.size())) {
        throw invalid_argument(
            "CVRPCodec: bitsPerKey is too small to encode a full "
            "permutation without ties."
        );
    }

   
    const int bits = static_cast<int>(bitsPerKey_);
    BinaryChromosome chromosome(
        vector<uint8_t>(binaryLength(), 0)
    );
    for (int p = 0; p < permutation.size(); ++p) {
        const int slot =
            static_cast<int>(permutation[p] - 1);
        chromosome.encodeUnsigned(
            slot * bits, bits, static_cast<uint32_t>(p)
        );
    }
    return chromosome;
}



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
    } catch (const invalid_argument&) {
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

vector<int> CVRPCodec::permutationFromKeys(
    const vector<double>& keys
) const {
    return orderByKey(keys);
}

vector<int> CVRPCodec::permutationFromKeys(
    const vector<uint32_t>& keys
) const {
    return orderByKey(keys);
}
