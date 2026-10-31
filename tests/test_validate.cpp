// Task 10 checks: validate with the (lo, hi) range method (Lecture 07, Section 6).
//   1. valid trees: empty, one node, the lecture tree, the file-order tree, and the
//      6,071-level chain from sorted input (the recursion goes 6,072 calls deep)
//   2. broken trees built by hand through AirportBSTTestAccess (below): the lecture's
//      Figure 3 and N1-N4, a duplicate key, swapped records, a wrong count. Each must fail
//      at the right node with the right range. The parent-child-only check (the lecture's
//      "common mistake") is run beside it to show which ones it misses.
// Run from the project root.

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "check.h"

// Declared a friend in AirportBST.h; defined only here. Rewires nodes without the BST rule.
struct AirportBSTTestAccess {
    using Node = AirportBST::Node;

    // Finds a node by looking at every node (not by the BST rule: the tree may be broken).
    static Node* nodeOf(AirportBST& tree, const std::string& code) {
        std::vector<Node*> stack;
        if (tree.root != nullptr) stack.push_back(tree.root);
        while (!stack.empty()) {
            Node* node = stack.back();
            stack.pop_back();
            if (node->data.getIata() == code) return node;
            if (node->left != nullptr) stack.push_back(node->left);
            if (node->right != nullptr) stack.push_back(node->right);
        }
        return nullptr;
    }

    // Hangs a new leaf below parentCode on the given side, whatever its key.
    static bool attachLeaf(AirportBST& tree, const std::string& parentCode, bool asLeft,
                           const Airport& airport) {
        Node* parent = nodeOf(tree, parentCode);
        if (parent == nullptr) return false;
        Node*& slot = asLeft ? parent->left : parent->right;
        if (slot != nullptr) return false;
        slot = new Node(airport);
        tree.count++;
        return true;
    }

    // Exchanges the records of two nodes; the links stay where they are.
    static bool swapRecords(AirportBST& tree, const std::string& a, const std::string& b) {
        Node* na = nodeOf(tree, a);
        Node* nb = nodeOf(tree, b);
        if (na == nullptr || nb == nullptr) return false;
        std::swap(na->data, nb->data);
        return true;
    }

    static void setCount(AirportBST& tree, int count) { tree.count = count; }
};

static Airport ap(const std::string& code) {
    return Airport("Test " + code, "City", "Country", code, "", 0.0, 0.0);
}

static void buildCodes(AirportBST& tree, const std::vector<std::string>& codes) {
    int n = static_cast<int>(codes.size());
    for (int i = 0; i < n; i++) tree.insert(ap(codes[i]));
}

// The lecture's WRONG check: each node compared only with its own two children.
static bool parentChildOnly(const AirportBST& tree) {
    std::vector<NodeInfo> nodes = tree.nodes();
    int n = static_cast<int>(nodes.size());
    for (int i = 0; i < n; i++) {
        if (!nodes[i].left.empty() && nodes[i].left >= nodes[i].code) return false;
        if (!nodes[i].right.empty() && nodes[i].right <= nodes[i].code) return false;
    }
    return true;
}

static void expectViolation(const char* label, const AirportBST& tree, const std::string& bad,
                            const std::string& low, const std::string& high, bool parentChildPasses) {
    ValidateResult v = tree.validate();
    CHECK(!v.ok);
    CHECK(v.badCode == bad && v.low == low && v.high == high);
    CHECK(parentChildOnly(tree) == parentChildPasses);
    int badIndex = 0;
    CHECK(!isSorted(tree.inorder(), badIndex));             // P1 agrees: inorder is not sorted
    std::cout << "  " << label << ": validate " << v.describe() << "; parent-child check says "
              << (parentChildPasses ? "valid (missed it)" : "invalid") << "\n";
}

static void testValidTrees() {
    AirportBST empty;
    ValidateResult v = empty.validate();
    CHECK(v.ok && v.reached == 0 && v.count == 0 && v.describe() == "OK");

    AirportBST one;
    buildCodes(one, {"K40"});
    CHECK(one.validate().ok && one.validate().reached == 1);

    AirportBST lecture;
    buildCodes(lecture, {"K40", "K20", "K60", "K10", "K30", "K50", "K70"});
    v = lecture.validate();
    CHECK(v.ok && v.reached == 7 && parentChildOnly(lecture));
}

static void testBrokenTrees() {
    std::cout << "broken trees (lecture Figure 3 and Section 1.3):\n";

    // Figure 3: 45 is larger than its parent 20 but sits in the LEFT subtree of 40.
    AirportBST fig3;
    buildCodes(fig3, {"K40", "K20", "K60", "K10"});
    CHECK(AirportBSTTestAccess::attachLeaf(fig3, "K20", false, ap("K45")));
    expectViolation("Figure 3", fig3, "K45", "K20", "K40", true);
    CHECK(fig3.validate().describe() == "FAILED (K45 is not in (K20, K40))");

    // N1: 8 is the right child of 10 but smaller.
    AirportBST n1;
    buildCodes(n1, {"K10", "K05"});
    CHECK(AirportBSTTestAccess::attachLeaf(n1, "K10", false, ap("K08")));
    expectViolation("N1", n1, "K08", "K10", "", false);
    CHECK(n1.validate().describe() == "FAILED (K08 is not in (K10, +inf))");

    // N2: 15 < 30 is fine for its parent, but 15 is in the right subtree of 20.
    AirportBST n2;
    buildCodes(n2, {"K20", "K10", "K30", "K40"});
    CHECK(AirportBSTTestAccess::attachLeaf(n2, "K30", true, ap("K15")));
    expectViolation("N2", n2, "K15", "K20", "K30", true);

    // N3: the running example with 20 and 30 swapped.
    AirportBST n3;
    buildCodes(n3, {"K40", "K20", "K60", "K10", "K30", "K50", "K70"});
    CHECK(AirportBSTTestAccess::swapRecords(n3, "K20", "K30"));
    expectViolation("N3", n3, "K20", "K30", "K40", false);

    // N4: 9 is in the left subtree of the root 8 while 9 > 8.
    AirportBST n4;
    buildCodes(n4, {"K08", "K03", "K10", "K01", "K06", "K04"});
    CHECK(AirportBSTTestAccess::attachLeaf(n4, "K06", false, ap("K09")));
    expectViolation("N4", n4, "K09", "K06", "K08", true);

    // A duplicate key: 20 with a left child 20 is not a BST (keys are distinct).
    AirportBST dup;
    buildCodes(dup, {"K20"});
    CHECK(AirportBSTTestAccess::attachLeaf(dup, "K20", true, ap("K20")));
    expectViolation("duplicate", dup, "K20", "", "K20", false);
    CHECK(dup.validate().describe() == "FAILED (K20 is not in (-inf, K20))");
}

static void testRealTrees(const std::vector<Airport>& records) {
    AirportBST tree;
    tree.buildFromArray(records);
    ValidateResult v = tree.validate();
    CHECK(v.ok && v.reached == 6072 && v.count == 6072);
    std::cout << "file-order tree: validate " << v.describe() << ", " << v.reached << " nodes reached\n";

    // Swap the root's record with its left child's: caught, then fixed again.
    CHECK(AirportBSTTestAccess::swapRecords(tree, "GKA", "AEY"));
    v = tree.validate();
    CHECK(!v.ok && !v.badCode.empty());
    std::cout << "  root and left child swapped: validate " << v.describe() << "\n";
    CHECK(AirportBSTTestAccess::swapRecords(tree, "GKA", "AEY"));
    CHECK(tree.validate().ok);

    // A count that does not match the nodes (a lost node would look like this).
    AirportBSTTestAccess::setCount(tree, 6073);
    v = tree.validate();
    CHECK(!v.ok && v.badCode.empty() && v.reached == 6072);
    CHECK(v.describe() == "FAILED (6072 nodes reached, count is 6073)");
    AirportBSTTestAccess::setCount(tree, 6072);
    CHECK(tree.validate().ok);

    // The sorted-order chain: IS-BST recurses once per level, 6,072 calls deep.
    std::vector<Airport> sorted = records;
    std::sort(sorted.begin(), sorted.end(),
              [](const Airport& a, const Airport& b) { return a.getIata() < b.getIata(); });
    AirportBST chain;
    chain.buildFromArray(sorted);
    CHECK(chain.height() == 6071);
    v = chain.validate();
    CHECK(v.ok && v.reached == 6072);
    std::cout << "sorted-order chain (height " << chain.height() << "): validate " << v.describe()
              << ", recursion depth " << v.reached << "\n";
}

int main() {
    testValidTrees();
    testBrokenTrees();

    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load) && records.size() == 6072);
    testRealTrees(records);
    return finishTests();
}
