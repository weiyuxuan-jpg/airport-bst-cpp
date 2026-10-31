// Task 12 checks (project Task 5): range FROM TO with pruning and the visited-node count.
//   1. range BOS BWI on the file-order tree: 113 airports (the project's expected number)
//   2. every result list compared with the brute-force answer (filter the inorder list)
//   3. the pruning, checked two independent ways:
//      a. exact count: a node is entered exactly when, on its path from the root, every
//         step left was below a key with FROM < key and every step right below a key with
//         key < TO; the number of such nodes, worked out from the tree's structure, must
//         equal the count range reports
//      b. bound: every visited node outside [FROM, TO] lies on the search path of FROM or
//         of TO, so visited - results <= |path(FROM)| + |path(TO)| (cost O(h + k))
//   4. 500 random ranges with a fixed seed, edge cases, and the 6,071-level chain
// Run from the project root.

#include <algorithm>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
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

// Everything the checks need to know about one tree, worked out once.
struct TreeFacts {
    std::vector<std::string> sortedCodes;             // inorder
    std::vector<std::vector<std::string>> paths;      // root -> node, one per node
};

static TreeFacts factsOf(const AirportBST& tree) {
    TreeFacts facts;
    facts.sortedCodes = codesOf(tree.inorder());
    std::vector<NodeInfo> nodes = tree.nodes();
    int n = static_cast<int>(nodes.size());
    for (int i = 0; i < n; i++) facts.paths.push_back(tree.find(nodes[i].code).path);
    return facts;
}

// The brute-force answer: the codes of the sorted list that lie in [from, to].
static std::vector<std::string> bruteForce(const TreeFacts& facts, const std::string& from, const std::string& to) {
    std::vector<std::string> out;
    int n = static_cast<int>(facts.sortedCodes.size());
    for (int i = 0; i < n; i++) {
        if (from <= facts.sortedCodes[i] && facts.sortedCodes[i] <= to) out.push_back(facts.sortedCodes[i]);
    }
    return out;
}

// Check (a): how many nodes the pruning rule must enter.
static int mustVisit(const TreeFacts& facts, const std::string& from, const std::string& to) {
    int count = 0;
    int n = static_cast<int>(facts.paths.size());
    for (int i = 0; i < n; i++) {
        const std::vector<std::string>& path = facts.paths[i];
        bool entered = true;
        int m = static_cast<int>(path.size());
        for (int j = 0; j < m - 1; j++) {
            bool wentLeft = path[j + 1] < path[j];
            if (!(wentLeft ? (from < path[j]) : (path[j] < to))) entered = false;
        }
        if (entered) count++;
    }
    return count;
}

// Runs one range and checks results, exact visited count and the O(h + k) bound.
static bool checkRange(const AirportBST& tree, const TreeFacts& facts, const std::string& from,
                       const std::string& to, RangeResult* out = nullptr) {
    RangeResult r = tree.range(from, to);
    if (out != nullptr) *out = r;
    bool resultsOk = codesOf(r.airports) == bruteForce(facts, from, to);
    bool exactCount = r.visited == mustVisit(facts, from, to);
    int outside = r.visited - static_cast<int>(r.airports.size());
    bool withinBound = outside <= tree.find(from).comparisons() + tree.find(to).comparisons();
    return resultsOk && exactCount && withinBound;
}

static void testLectureTree() {
    AirportBST t;
    t.buildFromArray({ap("K40"), ap("K20"), ap("K60"), ap("K10"), ap("K30"), ap("K50"), ap("K70")});
    TreeFacts f = factsOf(t);
    RangeResult r;
    CHECK(checkRange(t, f, "K25", "K55", &r));
    CHECK(codesOf(r.airports) == (std::vector<std::string>{"K30", "K40", "K50"}));
    CHECK(r.visited == 5);                                  // 40, 20, 30, 60, 50 (10 and 70 skipped)
    CHECK(checkRange(t, f, "K20", "K20", &r) && r.airports.size() == 1 && r.visited == 2);
    CHECK(checkRange(t, f, "K00", "K99", &r) && r.airports.size() == 7 && r.visited == 7);
    CHECK(checkRange(t, f, "K71", "K99", &r) && r.airports.empty() && r.visited == 3);   // 40, 60, 70
    r = t.range("K60", "K20");                              // FROM > TO
    CHECK(r.airports.empty() && r.visited == 0);
    AirportBST empty;
    r = empty.range("AAA", "ZZZ");
    CHECK(r.airports.empty() && r.visited == 0);
}

static void testRealTree(const AirportBST& tree, const TreeFacts& facts) {
    RangeResult r;
    CHECK(checkRange(tree, facts, "BOS", "BWI", &r));
    CHECK(r.airports.size() == 113);                         // the project's expected answer
    CHECK(r.visited == 127);                                 // the sample session's count
    CHECK(!r.airports.empty() && r.airports.front().getIata() == "BOS" && r.airports.back().getIata() == "BWI");
    int bad = 0;
    CHECK(isSorted(r.airports, bad));
    int outside = r.visited - static_cast<int>(r.airports.size());
    std::cout << "range BOS BWI: " << r.airports.size() << " airports, " << r.visited
              << " nodes visited (" << outside << " outside the range; |path(BOS)| = "
              << tree.find("BOS").comparisons() << ", |path(BWI)| = " << tree.find("BWI").comparisons()
              << "; " << tree.size() - r.visited << " of " << tree.size() << " nodes never looked at)\n";

    // Edge cases.
    CHECK(checkRange(tree, facts, "AAA", "ZZV", &r) && r.airports.size() == 6072 && r.visited == 6072);
    CHECK(checkRange(tree, facts, "000", "ZZZ", &r) && r.airports.size() == 6072);
    CHECK(checkRange(tree, facts, "XYZ", "XYZ", &r) && r.airports.empty() && r.visited == 14);   // = find XYZ
    CHECK(checkRange(tree, facts, "GKA", "GKA", &r) && r.airports.size() == 1);
    CHECK(checkRange(tree, facts, "000", "0ZZ", &r) && r.airports.empty());   // below every key
    CHECK(checkRange(tree, facts, "ZZW", "ZZZ", &r) && r.airports.empty());   // above every key
    CHECK(checkRange(tree, facts, "DU0", "DUZ", &r));        // contains DU9, the code with a digit
    bool hasDu9 = false;
    int n = static_cast<int>(r.airports.size());
    for (int i = 0; i < n; i++) {
        if (r.airports[i].getIata() == "DU9") hasDu9 = true;
    }
    CHECK(hasDu9);

    // 500 random ranges: ends drawn from the real codes and from made-up codes.
    std::mt19937 rng(2560);
    std::uniform_int_distribution<int> pick(0, static_cast<int>(facts.sortedCodes.size()) - 1);
    std::uniform_int_distribution<int> letter(0, 25);
    bool allOk = true;
    long long totalVisited = 0;
    long long totalResults = 0;
    for (int i = 0; i < 500; i++) {
        std::string a = facts.sortedCodes[pick(rng)];
        std::string b = facts.sortedCodes[pick(rng)];
        if (i % 2 == 1) {                                    // half of them: a made-up FROM
            a = std::string(1, static_cast<char>('A' + letter(rng))) +
                static_cast<char>('A' + letter(rng)) + static_cast<char>('A' + letter(rng));
        }
        if (a > b) std::swap(a, b);
        RangeResult rr;
        if (!checkRange(tree, facts, a, b, &rr)) {
            allOk = false;
            std::cout << "  FAILED for range " << a << " " << b << "\n";
        }
        totalVisited += rr.visited;
        totalResults += static_cast<long long>(rr.airports.size());
    }
    CHECK(allOk);
    std::cout << "500 random ranges: results = brute force, visited = exact prediction, within "
              << "the O(h + k) bound: " << (allOk ? "all OK" : "FAILED") << " (on average "
              << totalResults / 500 << " airports and " << totalVisited / 500 << " nodes visited)\n";
}

static void testChain(const std::vector<Airport>& records) {
    std::vector<Airport> sorted = records;
    std::sort(sorted.begin(), sorted.end(),
              [](const Airport& a, const Airport& b) { return a.getIata() < b.getIata(); });
    AirportBST chain;
    chain.buildFromArray(sorted);
    RangeResult r = chain.range("AAA", "ZZZ");              // recursion 6,072 deep
    CHECK(r.airports.size() == 6072 && r.visited == 6072);
    RangeResult b = chain.range("BOS", "BWI");
    CHECK(b.airports.size() == 113);
    std::cout << "sorted chain: range AAA ZZZ visited " << r.visited << " (recursion depth "
              << r.visited << "); range BOS BWI gives " << b.airports.size() << " airports but visits "
              << b.visited << " nodes\n";
}

int main() {
    testLectureTree();

    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load) && records.size() == 6072);
    AirportBST tree;
    tree.buildFromArray(records);
    TreeFacts facts = factsOf(tree);
    testRealTree(tree, facts);
    testChain(records);
    return finishTests();
}
