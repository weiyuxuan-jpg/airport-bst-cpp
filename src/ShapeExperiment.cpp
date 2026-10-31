#include "ShapeExperiment.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>

namespace {

using Clock = std::chrono::steady_clock;

double millisecondsSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

bool byCode(const Airport& a, const Airport& b) {
    return a.getIata() < b.getIata();
}

}  // namespace

std::vector<std::string> drawQueries(const std::vector<Airport>& records, int count, unsigned seed) {
    std::vector<std::string> queries;
    if (records.empty()) return queries;
    std::mt19937 rng(seed);
    unsigned n = static_cast<unsigned>(records.size());
    for (int i = 0; i < count; i++) queries.push_back(records[rng() % n].getIata());
    return queries;
}

std::vector<Airport> shuffledCopy(const std::vector<Airport>& records, unsigned seed) {
    std::vector<Airport> a = records;                             // step 1
    std::mt19937 rng(seed);
    int n = static_cast<int>(a.size());
    for (int i = 0; i < n - 1; i++) {                             // step 2
        int j = i + static_cast<int>(rng() % static_cast<unsigned>(n - i));   // step 3: i ... n - 1
        std::swap(a[i], a[j]);                                    // step 4
    }
    return a;                                                     // step 5
}

SeedSweep sweepSeeds(const std::vector<Airport>& records, unsigned firstSeed, int count) {
    SeedSweep sweep;
    double depthSum = 0.0;
    for (int s = 0; s < count; s++) {                             // step 1
        AirportBST tree;
        tree.buildFromArray(shuffledCopy(records, firstSeed + static_cast<unsigned>(s)));   // step 2
        sweep.heights.push_back(tree.height());                   // step 3
        depthSum += tree.averageDepth();
    }
    if (count > 0) {                                              // step 4
        std::vector<int> sorted = sweep.heights;
        std::sort(sorted.begin(), sorted.end());
        sweep.minHeight = sorted.front();
        sweep.medianHeight = sorted[sorted.size() / 2];
        sweep.maxHeight = sorted.back();
        sweep.meanAverageDepth = depthSum / count;
    }
    return sweep;
}

ShapeRow measureTree(const std::string& name, const AirportBST& tree, double buildMs,
                     const std::vector<std::string>& queries, int repeats) {
    ShapeRow row;
    row.name = name;
    row.height = tree.height();                                   // step 7
    row.averageDepth = tree.averageDepth();
    row.valid = tree.validate().ok;
    row.buildMs = buildMs;

    int q = static_cast<int>(queries.size());
    long long comparisons = 0;
    int found = 0;
    for (int i = 0; i < q; i++) {                                 // step 8: warm-up, not timed
        int c = 0;
        if (tree.countFind(queries[i], c)) found++;
        comparisons += c;
    }

    std::vector<double> times;
    for (int r = 0; r < repeats; r++) {                           // step 9
        long long runComparisons = 0;
        int runFound = 0;
        Clock::time_point start = Clock::now();                   // step 10
        for (int i = 0; i < q; i++) {                             // step 11
            int c = 0;
            if (tree.countFind(queries[i], c)) runFound++;        // step 12
            runComparisons += c;                                  // step 13
        }
        times.push_back(millisecondsSince(start));                // step 14
        comparisons = runComparisons;                             // the same in every run
        found = runFound;
    }
    std::sort(times.begin(), times.end());
    if (!times.empty()) {                                         // step 15
        row.searchMsMedian = times[times.size() / 2];
        row.searchMsMin = times.front();
        row.searchMsMax = times.back();
    }
    row.comparisonsPerSearch = (q > 0) ? static_cast<double>(comparisons) / q : 0.0;
    row.found = found;
    return row;
}

std::vector<ShapeRow> runShapeExperiment(const std::vector<Airport>& records, int queryCount,
                                         int repeats, unsigned seed) {
    std::vector<Airport> sorted = records;                        // step 1
    std::sort(sorted.begin(), sorted.end(), byCode);
    std::vector<std::string> queries = drawQueries(records, queryCount, seed);   // step 2
    std::vector<ShapeRow> rows;

    AirportBST fileOrder;                                         // step 3
    Clock::time_point start = Clock::now();
    fileOrder.buildFromArray(records);
    double buildMs = millisecondsSince(start);

    // Step 3b: at least 200 ms of untimed searches before any timing, so the first tree
    // measured is not timed while the processor is still speeding up.
    Clock::time_point warmStart = Clock::now();
    long long warmComparisons = 0;
    int q = static_cast<int>(queries.size());
    while (q > 0 && millisecondsSince(warmStart) < 200.0) {
        for (int i = 0; i < q; i++) {
            int c = 0;
            fileOrder.countFind(queries[i], c);
            warmComparisons += c;
        }
    }
    if (warmComparisons < 0) std::cout << "";                    // keeps the warm-up loop

    rows.push_back(measureTree("File order", fileOrder, buildMs, queries, repeats));

    AirportBST sortedOrder;                                       // step 4
    start = Clock::now();
    sortedOrder.buildFromArray(sorted);
    buildMs = millisecondsSince(start);
    rows.push_back(measureTree("Sorted order", sortedOrder, buildMs, queries, repeats));

    AirportBST balanced;                                          // step 5
    start = Clock::now();
    balanced.buildBalanced(sorted);
    buildMs = millisecondsSince(start);
    rows.push_back(measureTree("Balanced (middle rule)", balanced, buildMs, queries, repeats));

    std::vector<Airport> shuffled = shuffledCopy(records, seed);  // step 5b (not timed)
    AirportBST randomOrder;
    start = Clock::now();
    randomOrder.buildFromArray(shuffled);
    buildMs = millisecondsSince(start);
    rows.push_back(measureTree("Random shuffle", randomOrder, buildMs, queries, repeats));
    return rows;
}

void printSeedSweep(const SeedSweep& sweep, unsigned firstSeed, std::ostream& out) {
    int count = static_cast<int>(sweep.heights.size());
    out << "Random shuffle over " << count << " seeds (" << firstSeed << " to "
        << firstSeed + static_cast<unsigned>(count) - 1 << "): height min " << sweep.minHeight
        << ", median " << sweep.medianHeight << ", max " << sweep.maxHeight
        << "; mean average depth " << std::fixed << std::setprecision(2)
        << sweep.meanAverageDepth << "\n";
    out.unsetf(std::ios::fixed);
}

void printShapeTable(const std::vector<ShapeRow>& rows, int queryCount, int repeats,
                     unsigned seed, std::ostream& out) {
    out << queryCount << " random searches (seed " << seed << "), time = median of "
        << repeats << " runs\n";
    out << std::left << std::setw(24) << "Build order" << std::right << std::setw(7) << "Height"
        << std::setw(11) << "Avg depth" << std::setw(15) << "Search time" << std::setw(14)
        << "Comparisons" << std::setw(12) << "Build time" << "  Valid\n";
    int n = static_cast<int>(rows.size());
    for (int i = 0; i < n; i++) {
        const ShapeRow& r = rows[i];
        out << std::left << std::setw(24) << r.name << std::right << std::setw(7) << r.height
            << std::fixed << std::setprecision(1) << std::setw(11) << r.averageDepth
            << std::setprecision(3) << std::setw(12) << r.searchMsMedian << " ms"
            << std::setprecision(1) << std::setw(14) << r.comparisonsPerSearch
            << std::setprecision(2) << std::setw(9) << r.buildMs << " ms"
            << "  " << (r.valid ? "OK" : "FAILED") << "\n";
        out.unsetf(std::ios::fixed);
    }
    out << "(Comparisons = mean comparisons per search; it does not depend on the machine.)\n";
    out << "Search time over the " << repeats << " runs (min / median / max):\n";
    for (int i = 0; i < n; i++) {
        const ShapeRow& r = rows[i];
        out << "  " << std::left << std::setw(24) << r.name << std::right << std::fixed
            << std::setprecision(3) << r.searchMsMin << " / " << r.searchMsMedian << " / "
            << r.searchMsMax << " ms\n";
        out.unsetf(std::ios::fixed);
    }
}
