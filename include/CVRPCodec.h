#pragma once

#include "BinaryChromosome.h"
#include "FloatChromosome.h"
#include "IntegerChromosome.h"

#include <random>
#include <vector>

// A route is the ordered list of customer ids a vehicle visits.
// The depot (id 0) is implicit at the start and end of every route and is
// NOT stored; use CVRPCodec::withDepot() when you need {0, c1, ..., ck, 0}.
using Route = std::vector<int>;

// One entry per vehicle. An empty Route means that vehicle is not used.
using Routes = std::vector<Route>;

// Encoding / decoding between GA chromosomes and CVRP vehicle routes.
//
// ----------------------------------------------------------------------------
// CVRP model
// ----------------------------------------------------------------------------
//   * Depot            : id 0 (implicit, never stored in a chromosome).
//   * Customers        : ids 1 .. n.
//   * Vehicles         : K (the fleet size).
//   * Capacity         : NOT known to the codec. The codec only decides which
//                        customers go on which vehicle and in what order;
//                        the feasibility checker (Member 3) tests capacity on
//                        the decoded Routes.
//
// ----------------------------------------------------------------------------
// Permutation-with-separators model (used by all three representations)
// ----------------------------------------------------------------------------
// A "giant tour" is a permutation of L = n + K - 1 distinct values:
//     1 .. n          -> customers
//     n+1 .. n+K-1    -> separators (K-1 of them)
// Separators cut the tour into K segments; segment k is vehicle k's route.
//
//     n = 7, K = 3  ->  L = 9, separators are 8 and 9
//     [3 1 2 | 8 | 5 4 | 9 | 6 7]  ->  {3,1,2}, {5,4}, {6,7}
//
// Two adjacent separators (or a separator at either end) give an empty route,
// so every permutation decodes to exactly K routes in which every customer is
// served exactly once. Capacity is not guaranteed -- that is what the
// feasibility check and the infeasibility-handling strategy are for.
//
// How each representation stores that permutation:
//   Integer : the permutation itself (IntegerChromosome, permutation flag set,
//             range [1, L], length L). Works directly with order crossover,
//             swap and inversion mutation.
//   Float   : "random keys". Gene i (i = 0..L-1) is a real in [0, 1] that is
//             the sort key of value i+1. Sorting the values by ascending key
//             gives the permutation (ties broken by lower index). Any real
//             vector is decodable, so multi-point crossover and Gaussian
//             mutation can never break validity.
//   Binary  : same idea with integer keys. Each value i+1 owns `bitsPerKey`
//             consecutive bits (MSB first); chromosome length is
//             L * bitsPerKey. Sorted by ascending key, ties by lower index.
//             Any bit string is decodable, so bit-flip is always safe.
//
// All three decoders return the same Routes type, so the fitness function and
// the feasibility checker do not care which representation the GA uses.
class CVRPCodec {
public:
    // bitsPerKey = 0 picks a sensible default: max(8, ceil(log2(L))) so the
    // keys can always hold a full permutation without ties. Valid range for
    // an explicit value: 1..31.
    // Throws std::invalid_argument if numCustomers < 1 or numVehicles < 1.
    CVRPCodec(int numCustomers, int numVehicles, int bitsPerKey = 0);

    int numCustomers() const { return numCustomers_; }
    int numVehicles() const { return numVehicles_; }

    // L = numCustomers + numVehicles - 1.
    int permutationLength() const { return numCustomers_ + numVehicles_ - 1; }

    int bitsPerKey() const { return bitsPerKey_; }

    // Length of a BinaryChromosome for this problem: L * bitsPerKey.
    std::size_t binaryLength() const;

    // Permutation value classification (1-based values).
    bool isCustomer(int value) const {
        return value >= 1 && value <= numCustomers_;
    }
    bool isSeparator(int value) const {
        return value > numCustomers_ && value <= permutationLength();
    }

    // ---- Creation: random valid chromosomes ---------------------------------
    IntegerChromosome createInteger(std::mt19937& rng) const;  // permutation
    FloatChromosome createFloat(std::mt19937& rng) const;      // keys in [0,1]
    BinaryChromosome createBinary(std::mt19937& rng) const;    // random bits

    // ---- Decoding: chromosome -> routes (always exactly K routes) -----------
    // Throw std::invalid_argument if the chromosome has the wrong length,
    // wrong bounds, or (integer) is not a permutation of 1..L.
    Routes decode(const IntegerChromosome& chromosome) const;
    Routes decode(const FloatChromosome& chromosome) const;
    Routes decode(const BinaryChromosome& chromosome) const;

    // Decodes a raw giant tour (permutation of 1..L) into routes.
    Routes decodePermutation(const std::vector<int>& permutation) const;

    // ---- Encoding: routes -> chromosome -------------------------------------
    // `routes` may have fewer than K entries (missing vehicles are treated as
    // unused). Throws std::invalid_argument if there are more than K routes,
    // a customer id is outside 1..n, or a customer is missing / repeated.
    //
    // Round trip: decode(encodeX(routes)) == routes padded with empty routes
    // up to K entries.
    IntegerChromosome encodeInteger(const Routes& routes) const;
    FloatChromosome encodeFloat(const Routes& routes) const;
    BinaryChromosome encodeBinary(const Routes& routes) const;

    // Routes -> giant tour (permutation of 1..L). Same validation as above.
    std::vector<int> routesToPermutation(const Routes& routes) const;

    // Decodes whatever representation the GA is using, given only the base
    // class reference that fitness/feasibility functions receive. Returns
    // false (and leaves `routes` untouched) if the chromosome is not an
    // Integer/Float/Binary chromosome or does not fit this problem.
    bool tryDecode(const Chromosome& chromosome, Routes& routes) const;

    // {0, c1, ..., ck, 0}: a route with the depot added at both ends.
    static Route withDepot(const Route& route);

private:
    int numCustomers_;
    int numVehicles_;
    int bitsPerKey_;

    // Values 1..L ordered by ascending key (stable, so ties keep index order).
    std::vector<int> permutationFromKeys(
        const std::vector<double>& keys
    ) const;
    std::vector<int> permutationFromKeys(
        const std::vector<std::uint32_t>& keys
    ) const;
};
