#pragma once

#include "BinaryChromosome.h"
#include "FloatChromosome.h"
#include "IntegerChromosome.h"

#include <random>
#include <vector>

using namespace std;
using Route = vector<int>;

using Routes = vector<Route>;


class CVRPCodec {
public:
    
    CVRPCodec(int numCustomers, int numVehicles, int bitsPerKey = 0);

    int numCustomers() const { return numCustomers_; }
    int numVehicles() const { return numVehicles_; }

    int permutationLength() const { return numCustomers_ + numVehicles_ - 1; }

    int bitsPerKey() const { return bitsPerKey_; }

    int binaryLength() const;

    bool isCustomer(int value) const {
        return value >= 1 && value <= numCustomers_;
    }
    bool isSeparator(int value) const {
        return value > numCustomers_ && value <= permutationLength();
    }

    IntegerChromosome createInteger(mt19937& rng) const;  
    FloatChromosome createFloat(mt19937& rng) const;      
    BinaryChromosome createBinary(mt19937& rng) const;    

   
    Routes decode(const IntegerChromosome& chromosome) const;
    Routes decode(const FloatChromosome& chromosome) const;
    Routes decode(const BinaryChromosome& chromosome) const;


    Routes decodePermutation(const vector<int>& permutation) const;

    
    IntegerChromosome encodeInteger(const Routes& routes) const;
    FloatChromosome encodeFloat(const Routes& routes) const;
    BinaryChromosome encodeBinary(const Routes& routes) const;

    vector<int> routesToPermutation(const Routes& routes) const;

    
    bool tryDecode(const Chromosome& chromosome, Routes& routes) const;

    static Route withDepot(const Route& route);

private:
    int numCustomers_;
    int numVehicles_;
    int bitsPerKey_;

    vector<int> permutationFromKeys(
        const vector<double>& keys
    ) const;
    vector<int> permutationFromKeys(
        const vector<uint32_t>& keys
    ) const;
};
