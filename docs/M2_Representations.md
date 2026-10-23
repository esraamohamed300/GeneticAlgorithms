# M2 – Chromosome Representations & CVRP Encoding/Decoding

## Files

| File | What it is |
|---|---|
| `include/GeneChromosome.h` | Template base: `std::vector<T>` of genes + `isValid()`. Derives from `Chromosome`. |
| `include/BinaryChromosome.h`, `src/BinaryChromosome.cpp` | Binary representation |
| `include/IntegerChromosome.h`, `src/IntegerChromosome.cpp` | Integer representation (free integers or permutation) |
| `include/FloatChromosome.h`, `src/FloatChromosome.cpp` | Floating-point representation |
| `include/CVRPCodec.h`, `src/CVRPCodec.cpp` | CVRP encoder/decoder for all three representations |
| `tests/` | `test_representations.cpp`, `test_cvrp_codec.cpp`, `TestUtils.h` |

`Chromosome.h` (M1) is unchanged. Every class below is a `Chromosome`, so it can be
stored in `Population` and cloned through the base pointer.

## The three representations

| | Gene type | Valid values | Notes |
|---|---|---|---|
| `BinaryChromosome` | `std::uint8_t` | 0 or 1 | `flip(i)`, `decodeUnsigned/encodeUnsigned(start, bits)` for reading bit groups as integers |
| `IntegerChromosome` | `int` | `[lowerBound, upperBound]` inclusive | Permutation mode: each value of the range exactly once, length = range size |
| `FloatChromosome` | `double` | finite, `[lowerBound, upperBound]` inclusive | `clamp()` repairs genes after mutation |

Common API (all three): `size()`, `operator[]`, `at()`, `genes()`, `isValid()`, `clone()`,
`toString()`, and static `random(...)` factories that take a `std::mt19937&`
(so the GA's seed controls everything).

Constructors validate and throw `std::invalid_argument`. Element access
(`operator[]`, `genes()`) does **not** validate, so operators can build children via
`clone()` + overwrite and then call `isValid()`.

## CVRP model

- Depot = id `0`, **implicit** (never stored). Customers = `1..n`. Vehicles = `K`.
- A route is `std::vector<int>` of customer ids; `Routes` is one route per vehicle.
  `CVRPCodec::withDepot(route)` gives `{0, c1, ..., ck, 0}`.
- The codec knows nothing about demands or capacity. That is M3's check on the decoded `Routes`.

### Permutation-with-separators ("giant tour")

A giant tour is a permutation of `L = n + K - 1` values: `1..n` are customers and
`n+1..n+K-1` are separators that cut the tour into K routes.

```
n = 7, K = 3  ->  L = 9, separators 8 and 9
[3 1 2 | 8 | 5 4 | 9 | 6 7]   ->   {3,1,2}  {5,4}  {6,7}
```

Adjacent separators or a separator at either end give an **empty route** (unused vehicle).
**Every** permutation decodes to exactly K routes with every customer served exactly
once. The only way a decoded solution can be infeasible is a capacity violation.

| Representation | How the giant tour is stored | Length |
|---|---|---|
| Integer | The permutation itself, range `[1, L]`, permutation flag set | `L` |
| Float | Random keys in `[0,1]`: gene `i` is the sort key of value `i+1`; sort ascending | `L` |
| Binary | Random keys as bit groups: value `i+1` owns `bitsPerKey` bits (MSB first); sort ascending | `L * bitsPerKey` |

Ties (possible with binary keys) are broken by lower index, so decoding is deterministic.
Default `bitsPerKey = max(8, ceil(log2 L))`.

### API

```cpp
CVRPCodec codec(numCustomers, numVehicles);      // optional 3rd arg: bitsPerKey

IntegerChromosome c = codec.createInteger(rng);  // random permutation of 1..L
FloatChromosome   f = codec.createFloat(rng);    // random keys in [0,1]
BinaryChromosome  b = codec.createBinary(rng);   // random bits

Routes r = codec.decode(c);                      // also decode(f), decode(b)
IntegerChromosome e = codec.encodeInteger(r);    // also encodeFloat / encodeBinary
```

`decode` always returns exactly K routes and throws `std::invalid_argument` for a chromosome of the
wrong length/bounds or a broken permutation. `encode*` throws if routes have more than K entries,
an invalid or repeated customer id, or a missing customer.

Round trip: `decode(encodeX(routes))` equals `routes` padded with empty routes up to K.
Separators are interchangeable, so several integer chromosomes can decode to the same routes;
`encode` writes them in canonical order.

## Notes for teammates

**M4 (operators)**
- Integer/permutation chromosomes: use **order crossover** and **swap / inversion mutation**.
  Multi-point crossover would duplicate/lose customers there.
- Float and binary key chromosomes: **multi-point crossover**, **bit-flip** (binary) and
  uniform/Gaussian mutation (float, then call `clamp()`) can never produce an undecodable child.
- Operators can be templated on `GeneChromosome<T>`; the permutation flag, bounds and
  `isValid()` tell you which invariant to preserve.

**M3 (feasibility)**
- Decode with `codec.decode(...)`, then for each route sum the demands and compare to capacity.
- "Each customer served once" and "at most K vehicles" are guaranteed by the representation;
  checking them again is cheap and harmless.

**M5 (fitness/runner)**
- Fitness and feasibility functions receive `const Chromosome&`;
  `codec.tryDecode(chromosome, routes)` turns any of the three representations into `Routes`
  (returns false if it can't be decoded) for the distance calculation.
- Random initial solutions often violate capacity, so M1/M5 need to agree on penalty vs repair.

---

# M3 – Selection, Replacement & Feasibility Checking

| File | What it is |
|---|---|
| `include/Selection.h`, `src/Selection.cpp` | `SelectionMethod`, `TournamentSelection`, `RouletteWheelSelection` |
| `include/Replacement.h`, `src/Replacement.cpp` | `ReplacementMethod`, `GenerationalReplacement`, `SteadyStateReplacement` |
| `include/CVRPFeasibility.h`, `src/CVRPFeasibility.cpp` | `CVRPFeasibilityChecker`, `FeasibilityReport` |
| `tests/test_selection.cpp`, `test_replacement.cpp`, `test_feasibility.cpp` | unit tests |

## Fitness convention (agree on this with M1 and M5)

**Higher fitness = better.** Selection and replacement only see one `double` per individual,
so they don't care about the representation. A minimisation problem such as CVRP distance must be
converted before it reaches them, e.g. `fitness = 1 / (1 + cost)` (or `-cost`; tournament and
replacement work with negative values, roulette shifts them). If a penalty is used for infeasible
solutions, apply it to the cost before converting.

## Selection

```cpp
TournamentSelection tournament(3);            // default size 3
RouletteWheelSelection roulette;

size_t parent = tournament.select(fitness, rng);                  // one index
std::vector<size_t> parents = roulette.selectMany(fitness, 50, rng);  // 50 indices
```

- Tournament: draws `size` random individuals, returns the fittest (size 1 = uniform random).
- Roulette: probability proportional to fitness; negatives are shifted so the minimum becomes 0;
  zero-weight individuals are never picked; if all weights are 0 it falls back to uniform.
- Both throw `std::invalid_argument` on an empty or NaN/infinite fitness vector.
- They return indices into the population, so one individual can be picked several times.

## Replacement

The population size never changes.

```cpp
GenerationalReplacement gen(1);               // elitism = 1 (default); 0 = pure generational
SteadyStateReplacement  steady;               // onlyIfBetter = true (default)

gen.replace(population, fitness, offspring, offspringFitness);   // updates both in place
auto plan = gen.chooseSurvivors(fitness, offspringFitness);      // decision only, no chromosomes
```

- Generational: keep the `elitism` best parents, fill the rest with the best offspring
  (best remaining parents fill any gap if there are too few offspring).
- Steady-state: the best offspring replace the worst parents, one pair at a time; with
  `onlyIfBetter` an offspring is admitted only if strictly fitter than the parent it replaces,
  so the population never gets worse.
- `replace()` clones the survivors, leaves `offspring` untouched, and keeps `fitness[i]`
  aligned with `population[i]`.

## CVRP feasibility

```cpp
CVRPCodec codec(n, K);
CVRPFeasibilityChecker checker(codec, demands, capacity);  // demands has n+1 entries, demands[0] = 0

bool ok = checker.isFeasible(routes);                       // on decoded routes
bool ok2 = checker.isFeasible(chromosome);                  // any of the 3 representations
FeasibilityReport r = checker.check(routes);                // loads, overloaded routes, totalExcess, ...
FeasibilityFunction f = checker.asFunction();               // std::function<bool(const Chromosome&)> for GAEngine
```

A solution is feasible iff (1) there are at most K routes, (2) every customer 1..n is served exactly
once (no missing, duplicated or unknown ids; the depot 0 may not appear inside a route), and
(3) every route's total demand is <= capacity (tolerance 1e-9 for floating-point sums).
A chromosome that can't be decoded for this problem counts as infeasible.

Handy for M5's penalty approach: `FeasibilityReport::totalExcess` is the total overload summed over
all routes, and `hasUnservableCustomer()` flags instances with a customer bigger than the capacity
(no feasible solution exists).

## Build and test

```bash
CORE="src/BinaryChromosome.cpp src/IntegerChromosome.cpp src/FloatChromosome.cpp src/CVRPCodec.cpp \
      src/Population.cpp src/Selection.cpp src/Replacement.cpp src/CVRPFeasibility.cpp"
for t in representations cvrp_codec selection replacement feasibility; do
  g++ -std=c++17 -Wall -Wextra -Iinclude -Itests $CORE tests/test_$t.cpp -o test_$t && ./test_$t
done
```
