// Task 13 checks: balanced build (method B2, middle rule), rebalance, and the shape experiment.
//   1. the lecture's examples (Section 3.2 and in-class Q2)
//   2. every size n = 0 ... 300: height floor(log2 n) and the smallest possible total depth,
//      sum of floor(log2 i) for i = 1 ... n; unsorted or duplicate input is refused
//   3. the real data: balanced build, rebalance of the file-order tree, of the tree after
//      remove BOS (sample session: height 12, average depth 10.7) and of the sorted chain
//   4. the experiment: heights 177 / 6,071 / 12, every query found, and the mean
//      comparisons per search close to average depth + 1
// Run from the project root.

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "ShapeExperiment.h"
#include "check.h"

static Airport ap(const std::string& code) {
    return Airport("Test " + code, "City", "Country", code, "", 0.0, 0.0);
}

static std::vector<std::string> codesOf(const std::vector<Airport>& list) {
    std::vector<std::string> codes;
    int n = static_cast<int>(list.size());
    for (int i = 0; i < n; i++) codes.push_back(list[i].getIata());
    return codes;
}

// floor(log2 i) for i >= 1.
static int floorLog2(int i) {
    int k = 0;
    while (i > 1) {
        i /= 2;
        k++;
    }
    return k;
}

// The smallest possible total depth of a binary tree with n nodes.
static long long minimumTotalDepth(int n) {
    long long sum = 0;
    for (int i = 0; i < n; i++) sum += floorLog2(i + 1);
    return sum;
}

// n codes in increasing order: AAA, AAB, ...
static std::vector<Airport> sortedCodes(int n) {
    std::vector<Airport> out;
    for (int i = 0; i < n; i++) {
        std::string code = {static_cast<char>('A' + i / 676), static_cast<char>('A' + i / 26 % 26),
                            static_cast<char>('A' + i % 26)};
        out.push_back(ap(code));
    }
    return out;
}

static void testLectureExamples() {
    // Section 3.2: {10, ..., 70} -> preorder 40 20 10 30 60 50 70, height 2.
    AirportBST t;
    CHECK(t.buildBalanced({ap("K10"), ap("K20"), ap("K30"), ap("K40"), ap("K50"), ap("K60"), ap("K70")}));
    CHECK(codesOf(t.preorder()) == (std::vector<std::string>{"K40", "K20", "K10", "K30", "K60", "K50", "K70"}));
    CHECK(t.height() == 2 && t.validate().ok);

    // In-class Q2: {20, 30, 35, 40, 50, 60, 70, 80} -> root 40, height 3.
    AirportBST q;
    CHECK(q.buildBalanced({ap("K20"), ap("K30"), ap("K35"), ap("K40"), ap("K50"), ap("K60"), ap("K70"), ap("K80")}));
    CHECK(q.levelOrder() == (std::vector<std::string>{"K40", "K30", "K60", "K20", "K35", "K50", "K70", "K80"}));
    CHECK(q.height() == 3);
}

static void testEverySize() {
    bool allOk = true;
    for (int n = 0; n < 301; n++) {
        std::vector<Airport> list = sortedCodes(n);
        AirportBST t;
        if (!t.buildBalanced(list)) allOk = false;
        int wantHeight = (n == 0) ? -1 : floorLog2(n);
        double wantAverage = (n == 0) ? 0.0 : static_cast<double>(minimumTotalDepth(n)) / n;
        if (t.size() != n || t.height() != wantHeight || t.averageDepth() != wantAverage) allOk = false;
        if (!t.validate().ok || codesOf(t.inorder()) != codesOf(list)) allOk = false;
    }
    CHECK(allOk);
    std::cout << "balanced build for n = 0 ... 300: height floor(log2 n) and minimum total depth: "
              << (allOk ? "all OK" : "FAILED") << "\n";

    // Not strictly increasing: refused, and the tree that was there is kept.
    AirportBST t;
    t.buildBalanced(sortedCodes(5));
    std::vector<std::string> before = t.levelOrder();
    CHECK(!t.buildBalanced({ap("K20"), ap("K10")}));
    CHECK(!t.buildBalanced({ap("K10"), ap("K10")}));
    CHECK(t.levelOrder() == before && t.size() == 5);
}

static void testRealData(const std::vector<Airport>& records) {
    std::vector<Airport> sorted = records;
    std::sort(sorted.begin(), sorted.end(),
              [](const Airport& a, const Airport& b) { return a.getIata() < b.getIata(); });

    AirportBST balanced;
    CHECK(balanced.buildBalanced(sorted));
    CHECK(balanced.height() == 12);                                  // floor(log2 6072)
    CHECK(balanced.averageDepth() == static_cast<double>(minimumTotalDepth(6072)) / 6072);
    CHECK(balanced.validate().ok && codesOf(balanced.inorder()) == codesOf(sorted));
    std::cout << "balanced build of 6072: height " << balanced.height() << ", average depth "
              << balanced.averageDepth() << " (minimum possible " << minimumTotalDepth(6072) << " / 6072)\n";

    // rebalance the file-order tree: same airports, height 12, and doing it twice changes nothing.
    AirportBST tree;
    tree.buildFromArray(records);
    std::vector<std::string> inorderBefore = codesOf(tree.inorder());
    tree.rebalance();
    CHECK(tree.height() == 12 && tree.validate().ok && tree.size() == 6072);
    CHECK(codesOf(tree.inorder()) == inorderBefore);
    CHECK(tree.levelOrder() == balanced.levelOrder());               // the same tree as a direct build
    std::vector<std::string> once = tree.levelOrder();
    tree.rebalance();
    CHECK(tree.levelOrder() == once);

    // The sample session: remove BOS, then rebalance -> height 12, average depth 10.7.
    AirportBST sample;
    sample.buildFromArray(records);
    sample.remove("BOS");
    sample.rebalance();
    CHECK(sample.size() == 6071 && sample.height() == 12);
    CHECK(sample.averageDepth() == static_cast<double>(minimumTotalDepth(6071)) / 6071);
    CHECK(std::round(sample.averageDepth() * 10) / 10 == 10.7);
    CHECK(sample.validate().ok && !sample.find("BOS").found);
    std::cout << "remove BOS + rebalance: height " << sample.height() << ", average depth "
              << sample.averageDepth() << " (sample session: 12 and 10.7)\n";

    // rebalance the 6,071-level chain (inorder uses an explicit stack).
    AirportBST chain;
    chain.buildFromArray(sorted);
    chain.rebalance();
    CHECK(chain.height() == 12 && chain.validate().ok && chain.levelOrder() == balanced.levelOrder());

    AirportBST empty;
    empty.rebalance();
    CHECK(empty.isEmpty() && empty.height() == -1);
}

static void testExperiment(const std::vector<Airport>& records) {
    std::vector<std::string> a = drawQueries(records, 100, 2560);
    std::vector<std::string> b = drawQueries(records, 100, 2560);
    std::vector<std::string> c = drawQueries(records, 100, 2561);
    CHECK(a == b && a != c);                                         // a fixed seed repeats exactly

    // The shuffle is a permutation (every record once) and repeats exactly for a seed.
    std::vector<Airport> s1 = shuffledCopy(records, 7);
    std::vector<Airport> s2 = shuffledCopy(records, 7);
    std::vector<std::string> k1 = codesOf(s1), k2 = codesOf(s2), k0 = codesOf(records);
    CHECK(k1 == k2 && k1 != k0);
    std::sort(k1.begin(), k1.end());
    std::sort(k0.begin(), k0.end());
    CHECK(k1 == k0);

    std::vector<ShapeRow> rows = runShapeExperiment(records, 10000, 1, 2560);
    CHECK(rows.size() == 4);
    if (rows.size() != 4) return;
    CHECK(rows[0].name == "File order" && rows[0].height == 177);
    CHECK(rows[1].name == "Sorted order" && rows[1].height == 6071 && rows[1].averageDepth == 3035.5);
    CHECK(rows[2].name == "Balanced (middle rule)" && rows[2].height == 12);
    CHECK(rows[3].name == "Random shuffle" && rows[3].height >= 20 && rows[3].height <= 40);
    AirportBST shuffledTree;                                         // the row is the seed-2560 shuffle
    shuffledTree.buildFromArray(shuffledCopy(records, 2560));
    CHECK(rows[3].height == shuffledTree.height() && rows[3].averageDepth == shuffledTree.averageDepth());
    std::cout << "experiment (1 timed run here): comparisons per search vs average depth + 1:\n";
    for (int i = 0; i < 4; i++) {
        const ShapeRow& r = rows[i];
        CHECK(r.valid && r.found == 10000 && r.searchMsMedian > 0.0);
        double expected = r.averageDepth + 1.0;
        CHECK(std::fabs(r.comparisonsPerSearch - expected) <= 0.05 * expected);
        std::cout << "  " << r.name << ": " << r.comparisonsPerSearch << " vs " << expected << "\n";
    }
    CHECK(rows[2].comparisonsPerSearch < rows[0].comparisonsPerSearch &&
          rows[0].comparisonsPerSearch < rows[1].comparisonsPerSearch);
    CHECK(rows[1].searchMsMedian > rows[2].searchMsMedian);         // 260 times the comparisons

    // Exact, not just close: COUNT-FIND gives FIND's path length for every code in the tree
    // and for a missing one, and the experiment's mean is the mean path length of its queries.
    AirportBST fileTree;
    fileTree.buildFromArray(records);
    bool sameAsFind = true;
    int n = static_cast<int>(records.size());
    for (int i = 0; i < n; i++) {
        int c = 0;
        bool found = fileTree.countFind(records[i].getIata(), c);
        if (!found || c != fileTree.find(records[i].getIata()).comparisons()) sameAsFind = false;
    }
    int missing = 0;
    CHECK(!fileTree.countFind("XYZ", missing) && missing == fileTree.find("XYZ").comparisons());
    CHECK(sameAsFind);
    std::vector<std::string> queries = drawQueries(records, 10000, 2560);
    long long total = 0;
    for (int i = 0; i < 10000; i++) total += fileTree.find(queries[i]).comparisons();
    ShapeRow row = measureTree("check", fileTree, 0.0, queries, 1);
    CHECK(row.comparisonsPerSearch == static_cast<double>(total) / 10000);
    CHECK(row.comparisonsPerSearch == rows[0].comparisonsPerSearch);   // same queries as the experiment
}

int main() {
    testLectureExamples();
    testEverySize();

    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load) && records.size() == 6072);
    testRealData(records);
    testExperiment(records);
    return finishTests();
}
