// Task 6 checks: find with search path and comparison count, minimum and maximum.
//   1. the lecture's search example (Lecture 07, Section 4.2) and the empty tree
//   2. the six search cases of the project, Task 2(b), on the file-order tree:
//      root, a leaf, the deepest node, a missing code (XYZ), a code smaller than every
//      key and one larger than every key
//   3. every one of the 6,072 codes: the path must follow the BST rule at each step and
//      the comparisons must be depth + 1; their total must reproduce the average depth
// Run from the project root.

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "check.h"

static Airport ap(const std::string& code) {
    return Airport("Test " + code, "City", "Country", code, "", 0.0, 0.0);
}

using NodeMap = std::map<std::string, NodeInfo>;

// True if path is exactly the route a search for target must take: it starts at the root
// and each next code is the left child when target is smaller, else the right child.
// If found, the path ends at target; if not, the last node has no child on target's side.
static bool pathFollowsBstRule(const FindResult& r, const std::string& target,
                               const std::string& rootCode, const NodeMap& byCode) {
    int n = r.comparisons();
    if (n == 0) return rootCode.empty() && !r.found;
    if (r.path[0] != rootCode) return false;
    for (int i = 0; i < n; i++) {
        auto it = byCode.find(r.path[i]);
        if (it == byCode.end()) return false;
        const NodeInfo& node = it->second;
        if (target == node.code) return r.found && i == n - 1;
        const std::string& next = (target < node.code) ? node.left : node.right;
        if (i == n - 1) return !r.found && next.empty();   // fell off at a NULL child
        if (r.path[i + 1] != next) return false;
    }
    return false;
}

static void testLectureExample() {
    AirportBST tree;
    tree.buildFromArray({ap("K40"), ap("K20"), ap("K60"), ap("K10"), ap("K30"), ap("K50"), ap("K70")});

    FindResult r50 = tree.find("K50");                              // path 40 -> 60 -> 50
    CHECK(r50.found && r50.path == (std::vector<std::string>{"K40", "K60", "K50"}));
    CHECK(r50.comparisons() == 3 && r50.airport.getName() == "Test K50");
    FindResult r35 = tree.find("K35");                              // path 40 -> 20 -> 30 -> NULL
    CHECK(!r35.found && r35.path == (std::vector<std::string>{"K40", "K20", "K30"}));

    Airport lo, hi;
    CHECK(tree.minimum(lo) && lo.getIata() == "K10");
    CHECK(tree.maximum(hi) && hi.getIata() == "K70");

    AirportBST empty;
    FindResult none = empty.find("BOS");
    CHECK(!none.found && none.comparisons() == 0);
    CHECK(!empty.minimum(lo) && !empty.maximum(hi));
    CHECK(empty.nodes().empty());
}

static void report(const char* label, const std::string& code, const FindResult& r) {
    std::cout << "  " << label << " " << code << ": " << (r.found ? "found" : "not found")
              << ", comparisons " << r.comparisons();
    if (!r.path.empty()) {
        std::cout << ", path " << r.path.front() << " -> ... -> " << r.path.back()
                  << (r.found ? "" : " -> (NULL)");
    }
    std::cout << "\n";
}

static void testRealData() {
    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load));
    AirportBST tree;
    tree.buildFromArray(records);

    std::vector<NodeInfo> nodes = tree.nodes();
    CHECK(static_cast<int>(nodes.size()) == tree.size());
    if (nodes.empty()) return;
    NodeMap byCode;
    int n = static_cast<int>(nodes.size());
    for (int i = 0; i < n; i++) byCode[nodes[i].code] = nodes[i];
    const std::string rootCode = nodes[0].code;

    // Pick the cases from this tree: the shallowest leaf and the deepest node.
    int leaf = -1;
    int deepest = 0;
    for (int i = 0; i < n; i++) {
        if (leaf < 0 && nodes[i].left.empty() && nodes[i].right.empty()) leaf = i;
        if (nodes[i].depth > nodes[deepest].depth) deepest = i;
    }
    CHECK(leaf >= 0);
    if (leaf < 0) return;
    std::cout << "search cases on the file-order tree:\n";

    // 1. the root
    FindResult root = tree.find(rootCode);
    CHECK(rootCode == "GKA" && root.found && root.comparisons() == 1);
    report("root   ", rootCode, root);

    // 2. a leaf
    FindResult rLeaf = tree.find(nodes[leaf].code);
    CHECK(rLeaf.found && rLeaf.comparisons() == nodes[leaf].depth + 1);
    CHECK(pathFollowsBstRule(rLeaf, nodes[leaf].code, rootCode, byCode));
    report("leaf   ", nodes[leaf].code, rLeaf);

    // 3. the deepest node: depth = height, so height + 1 comparisons
    FindResult rDeep = tree.find(nodes[deepest].code);
    CHECK(nodes[deepest].depth == 177);
    CHECK(rDeep.found && rDeep.comparisons() == 178);
    CHECK(pathFollowsBstRule(rDeep, nodes[deepest].code, rootCode, byCode));
    report("deepest", nodes[deepest].code, rDeep);

    // The sample session's example: find BOS takes 12 comparisons.
    FindResult bos = tree.find("BOS");
    CHECK(bos.found && bos.comparisons() == 12);
    CHECK(bos.airport.summary() ==
          "BOS  General Edward Lawrence Logan International Airport, Boston, United States");
    CHECK(pathFollowsBstRule(bos, "BOS", rootCode, byCode));
    report("sample ", "BOS", bos);

    // 4. a code that does not exist
    FindResult xyz = tree.find("XYZ");
    CHECK(!xyz.found && pathFollowsBstRule(xyz, "XYZ", rootCode, byCode));
    report("missing", "XYZ", xyz);

    // 5. and 6. smaller than every key / larger than every key: the search follows the
    // leftmost / rightmost path and falls off just past the minimum / maximum.
    Airport lo, hi;
    CHECK(tree.minimum(lo) && lo.getIata() == "AAA");
    CHECK(tree.maximum(hi) && hi.getIata() == "ZZV");
    FindResult small = tree.find("AA0");                            // '0' < 'A', so AA0 < AAA
    CHECK(!small.found && small.path.back() == lo.getIata());
    CHECK(small.comparisons() == byCode[lo.getIata()].depth + 1);
    CHECK(pathFollowsBstRule(small, "AA0", rootCode, byCode));
    report("smaller", "AA0", small);
    FindResult large = tree.find("ZZZ");
    CHECK(!large.found && large.path.back() == hi.getIata());
    CHECK(large.comparisons() == byCode[hi.getIata()].depth + 1);
    CHECK(pathFollowsBstRule(large, "ZZZ", rootCode, byCode));
    report("larger ", "ZZZ", large);
    std::cout << "  minimum " << lo.getIata() << ", maximum " << hi.getIata() << "\n";

    // minimum / maximum agree with the ends of the inorder list.
    std::vector<Airport> list = tree.inorder();
    CHECK(list.front().getIata() == lo.getIata() && list.back().getIata() == hi.getIata());

    // Every code: right path, depth + 1 comparisons. Sum of (comparisons - 1) = sum of depths.
    long long totalComparisons = 0;
    int maxComparisons = 0;
    bool allFound = true;
    bool allPathsValid = true;
    bool allDepthsMatch = true;
    for (int i = 0; i < n; i++) {
        FindResult r = tree.find(nodes[i].code);
        if (!r.found) allFound = false;
        if (!pathFollowsBstRule(r, nodes[i].code, rootCode, byCode)) allPathsValid = false;
        if (r.comparisons() != nodes[i].depth + 1) allDepthsMatch = false;
        totalComparisons += r.comparisons();
        if (r.comparisons() > maxComparisons) maxComparisons = r.comparisons();
    }
    CHECK(allFound && allPathsValid && allDepthsMatch);
    CHECK(maxComparisons - 1 == tree.height());
    double averageFromSearches = static_cast<double>(totalComparisons - n) / n;
    CHECK(averageFromSearches == tree.averageDepth());
    std::cout << "  all " << n << " codes found by valid paths; mean comparisons "
              << static_cast<double>(totalComparisons) / n << ", so average depth "
              << averageFromSearches << "\n";
}

int main() {
    testLectureExample();
    testRealData();
    return finishTests();
}
