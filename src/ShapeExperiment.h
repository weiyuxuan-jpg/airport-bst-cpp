#ifndef SHAPE_EXPERIMENT_H
#define SHAPE_EXPERIMENT_H

#include <iosfwd>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"

// The project's Task 6: the same records built into trees in different orders, with the
// height, the average depth and the time for a fixed set of random searches
// (docs/pseudocode.md, section 11).

// One row of the table.
struct ShapeRow {
    std::string name;                 // "File order", ...
    int height = 0;
    double averageDepth = 0.0;
    bool valid = false;               // validate() of the built tree
    double buildMs = 0.0;             // building the tree once (not part of the search time)
    double searchMsMedian = 0.0;      // all queries, median of the timed runs
    double searchMsMin = 0.0;
    double searchMsMax = 0.0;
    double comparisonsPerSearch = 0.0;
    int found = 0;                    // queries that were found (all of them should be)
};

// Random choices use std::mt19937 directly (its output is fixed by the C++ standard) with
// "mod", not std::shuffle / uniform_int_distribution (which differ between standard
// libraries), so a seed gives the same results with every compiler.

// count codes drawn uniformly at random from records with the given seed.
std::vector<std::string> drawQueries(const std::vector<Airport>& records, int count, unsigned seed);

// SHUFFLE: a copy of records in random order (Fisher-Yates, front to back).
std::vector<Airport> shuffledCopy(const std::vector<Airport>& records, unsigned seed);

// SEED-SWEEP: heights and average depths of the trees built from count shuffles
// (seeds firstSeed, firstSeed + 1, ...).
struct SeedSweep {
    std::vector<int> heights;            // one per seed, in seed order
    int minHeight = 0;
    int medianHeight = 0;
    int maxHeight = 0;
    double meanAverageDepth = 0.0;
};
SeedSweep sweepSeeds(const std::vector<Airport>& records, unsigned firstSeed, int count);

// Measures a built tree: one untimed warm-up pass over queries, then repeats timed passes.
ShapeRow measureTree(const std::string& name, const AirportBST& tree, double buildMs,
                     const std::vector<std::string>& queries, int repeats);

// SHAPE-EXPERIMENT: rows for file order, sorted order, the balanced (middle rule) build and
// a random shuffle; seed is used both for the queries and for the shuffle.
std::vector<ShapeRow> runShapeExperiment(const std::vector<Airport>& records, int queryCount,
                                         int repeats, unsigned seed);

// Prints the rows as a table.
void printShapeTable(const std::vector<ShapeRow>& rows, int queryCount, int repeats,
                     unsigned seed, std::ostream& out);

// Prints a seed sweep in one line.
void printSeedSweep(const SeedSweep& sweep, unsigned firstSeed, std::ostream& out);

#endif
