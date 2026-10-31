// Task 7 checks: remove with cases D1, D2 (left only, right only), D3 (successor) and the root.
//   1. the lecture's delete example and in-class Q4, compared node by node
//   2. small trees for the cases the lecture does not show: D2 with a left child, D3 where
//      the successor is the right child itself, every case at the root, not found
//   3. the file-order tree: one code per case picked from this tree, each on a fresh tree.
//      After each delete: the code is gone, inorder is the old list minus that code (so the
//      tree is still a BST, P1), and every other node's depth is exactly as predicted:
//        D1 nothing moves; D2 the child's subtree moves up one level;
//        D3 the successor takes the deleted node's depth, its right subtree moves up one
//   4. deleting all 6,072 codes in a shuffled order
// Run from the project root.

#include <algorithm>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "check.h"

static Airport ap(const std::string& code) {
    return Airport("Test " + code, "City", "Country", code, "", 0.0, 0.0);
}

static void buildCodes(AirportBST& tree, const std::vector<std::string>& codes) {
    int n = static_cast<int>(codes.size());
    for (int i = 0; i < n; i++) tree.insert(ap(codes[i]));
}

// The tree breadth-first as "code(left,right)" with "-" for a missing child.
static std::string shapeOf(const AirportBST& tree) {
    std::vector<NodeInfo> nodes = tree.nodes();
    std::string out;
    int n = static_cast<int>(nodes.size());
    for (int i = 0; i < n; i++) {
        if (i > 0) out += " ";
        out += nodes[i].code + "(" + (nodes[i].left.empty() ? "-" : nodes[i].left) + "," +
               (nodes[i].right.empty() ? "-" : nodes[i].right) + ")";
    }
    return out;
}

static std::vector<std::string> codesOf(const std::vector<Airport>& list) {
    std::vector<std::string> codes;
    int n = static_cast<int>(list.size());
    for (int i = 0; i < n; i++) codes.push_back(list[i].getIata());
    return codes;
}

static void testLectureExamples() {
    // Lecture 07, Section 5.2: the running tree with 35 inserted; delete 10, 30, 40.
    AirportBST t;
    buildCodes(t, {"K40", "K20", "K60", "K10", "K30", "K50", "K70", "K35"});
    RemoveResult r = t.remove("K10");
    CHECK(r.kind == DeleteCase::Leaf && !r.wasRoot);
    CHECK(shapeOf(t) == "K40(K20,K60) K20(-,K30) K60(K50,K70) K30(-,K35) K50(-,-) K70(-,-) K35(-,-)");
    r = t.remove("K30");
    CHECK(r.kind == DeleteCase::OnlyRightChild && r.other == "K35" && !r.wasRoot);
    CHECK(r.describe() == "case D2: only a right child, K35");
    r = t.remove("K40");
    CHECK(r.kind == DeleteCase::TwoChildren && r.other == "K50" && r.wasRoot);
    CHECK(r.describe() == "case D3: two children, successor K50");
    CHECK(shapeOf(t) == "K50(K20,K60) K20(-,K35) K60(-,K70) K35(-,-) K70(-,-)");   // preorder 50 20 35 60 70
    CHECK(t.size() == 5);
    CHECK(t.validate().ok && t.validate().reached == 5);

    // In-class Q4: {50, 30, 70, 20, 40, 60, 80, 35}; delete 20, 30, 50.
    AirportBST q;
    buildCodes(q, {"K50", "K30", "K70", "K20", "K40", "K60", "K80", "K35"});
    CHECK(q.remove("K20").kind == DeleteCase::Leaf);
    r = q.remove("K30");
    CHECK(r.kind == DeleteCase::OnlyRightChild && r.other == "K40");
    r = q.remove("K50");
    CHECK(r.kind == DeleteCase::TwoChildren && r.other == "K60" && r.wasRoot);
    CHECK(shapeOf(q) == "K60(K40,K70) K40(K35,-) K70(-,K80) K35(-,-) K80(-,-)");
    CHECK(q.validate().ok);
}

static void testSmallCases() {
    // D2 with only a left child.
    AirportBST a;
    buildCodes(a, {"K40", "K20", "K10"});
    RemoveResult r = a.remove("K20");
    CHECK(r.kind == DeleteCase::OnlyLeftChild && r.other == "K10" && !r.wasRoot);
    CHECK(r.describe() == "case D2: only a left child, K10");
    CHECK(shapeOf(a) == "K40(K10,-) K10(-,-)");

    // D3 where the successor is the right child itself (it has no left child), at the root:
    // the root's right link, not the successor parent's left link, must be updated.
    AirportBST b;
    buildCodes(b, {"K40", "K20", "K60", "K70"});
    r = b.remove("K40");
    CHECK(r.kind == DeleteCase::TwoChildren && r.other == "K60" && r.wasRoot);
    CHECK(shapeOf(b) == "K60(K20,K70) K20(-,-) K70(-,-)");

    // The same situation below the root.
    AirportBST c;
    buildCodes(c, {"K80", "K40", "K20", "K60", "K70"});
    r = c.remove("K40");
    CHECK(r.kind == DeleteCase::TwoChildren && r.other == "K60" && !r.wasRoot);
    CHECK(shapeOf(c) == "K80(K60,-) K60(K20,K70) K20(-,-) K70(-,-)");
    CHECK(b.validate().ok && c.validate().ok);

    // Every case at the root.
    AirportBST one;
    buildCodes(one, {"K40"});
    r = one.remove("K40");
    CHECK(r.kind == DeleteCase::Leaf && r.wasRoot);
    CHECK(one.isEmpty() && one.size() == 0 && one.height() == -1);
    AirportBST leftOnly;
    buildCodes(leftOnly, {"K40", "K20", "K10"});
    r = leftOnly.remove("K40");
    CHECK(r.kind == DeleteCase::OnlyLeftChild && r.wasRoot && shapeOf(leftOnly) == "K20(K10,-) K10(-,-)");
    AirportBST rightOnly;
    buildCodes(rightOnly, {"K40", "K60", "K70"});
    r = rightOnly.remove("K40");
    CHECK(r.kind == DeleteCase::OnlyRightChild && r.wasRoot && shapeOf(rightOnly) == "K60(-,K70) K70(-,-)");

    // Not found: nothing changes.
    AirportBST empty;
    CHECK(empty.remove("BOS").kind == DeleteCase::NotFound);
    std::string before = shapeOf(b);
    r = b.remove("K99");
    CHECK(r.kind == DeleteCase::NotFound && r.describe() == "not found");
    CHECK(shapeOf(b) == before && b.size() == 3);

    // A deleted code can be inserted again.
    CHECK(b.insert(ap("K40")) && b.find("K40").found && b.size() == 4);
}

using NodeMap = std::map<std::string, NodeInfo>;

static std::set<std::string> subtreeOf(const NodeMap& byCode, const std::string& top) {
    std::set<std::string> out;
    if (top.empty()) return out;
    std::vector<std::string> todo = {top};
    while (!todo.empty()) {
        std::string code = todo.back();
        todo.pop_back();
        out.insert(code);
        const NodeInfo& node = byCode.at(code);
        if (!node.left.empty()) todo.push_back(node.left);
        if (!node.right.empty()) todo.push_back(node.right);
    }
    return out;
}

// Deletes code from a fresh file-order tree and checks the whole result.
static void checkCaseOnRealTree(const std::vector<Airport>& records, const std::string& label,
                                const std::string& code, DeleteCase expected) {
    AirportBST tree;
    tree.buildFromArray(records);
    std::vector<NodeInfo> beforeNodes = tree.nodes();
    NodeMap before;
    int n = static_cast<int>(beforeNodes.size());
    for (int i = 0; i < n; i++) before[beforeNodes[i].code] = beforeNodes[i];
    std::vector<std::string> sortedBefore = codesOf(tree.inorder());
    int heightBefore = tree.height();
    const NodeInfo x = before.at(code);

    RemoveResult r = tree.remove(code);
    CHECK(r.kind == expected);
    CHECK(r.wasRoot == (code == beforeNodes[0].code));

    // Gone, and inorder is the old list minus code: still a BST with nothing else lost.
    CHECK(tree.size() == n - 1);
    CHECK(!tree.find(code).found);
    std::vector<std::string> expectedList = sortedBefore;
    expectedList.erase(std::find(expectedList.begin(), expectedList.end(), code));
    std::vector<Airport> after = tree.inorder();
    int bad = 0;
    CHECK(codesOf(after) == expectedList && isSorted(after, bad));
    ValidateResult v = tree.validate();                         // Task 10 re-run
    CHECK(v.ok && v.reached == n - 1);

    // Who moves: the nodes whose depth drops by one, and (D3) the successor.
    std::set<std::string> movesUp;
    std::string successor;
    if (expected == DeleteCase::OnlyLeftChild) {
        CHECK(r.other == x.left);
        movesUp = subtreeOf(before, x.left);
    } else if (expected == DeleteCase::OnlyRightChild) {
        CHECK(r.other == x.right);
        movesUp = subtreeOf(before, x.right);
    } else if (expected == DeleteCase::TwoChildren) {
        // The successor is the next code in sorted order.
        int pos = static_cast<int>(std::find(sortedBefore.begin(), sortedBefore.end(), code) - sortedBefore.begin());
        successor = sortedBefore[pos + 1];
        CHECK(r.other == successor);
        CHECK(before.at(successor).left.empty());              // it has no left child
        movesUp = subtreeOf(before, before.at(successor).right);
    }
    std::vector<NodeInfo> afterNodes = tree.nodes();
    std::map<std::string, int> depthAfter;
    int m = static_cast<int>(afterNodes.size());
    for (int i = 0; i < m; i++) depthAfter[afterNodes[i].code] = afterNodes[i].depth;
    bool depthsAsPredicted = static_cast<int>(depthAfter.size()) == n - 1;
    for (int i = 0; i < n; i++) {
        const std::string& c = beforeNodes[i].code;
        if (c == code) continue;
        int want = beforeNodes[i].depth - (movesUp.count(c) ? 1 : 0);
        if (c == successor) want = x.depth;
        if (depthAfter.count(c) == 0 || depthAfter[c] != want) depthsAsPredicted = false;
    }
    CHECK(depthsAsPredicted);

    std::cout << "  " << label << " " << code << " (depth " << x.depth << "): " << r.describe()
              << (r.wasRoot ? ", was the root, new root " + afterNodes[0].code : "") << "; "
              << movesUp.size() << " nodes moved up; height " << heightBefore << " -> "
              << tree.height() << "; validate " << v.describe() << "\n";
}

static void testRealTree(const std::vector<Airport>& records) {
    AirportBST tree;
    tree.buildFromArray(records);
    std::vector<NodeInfo> nodes = tree.nodes();
    std::string leaf, leftOnly, rightOnly, twoChildren;
    int n = static_cast<int>(nodes.size());
    for (int i = 0; i < n; i++) {
        if (i == 0) continue;                                  // the root is tested on its own
        const NodeInfo& v = nodes[i];
        if (leaf.empty() && v.left.empty() && v.right.empty()) leaf = v.code;
        if (leftOnly.empty() && !v.left.empty() && v.right.empty()) leftOnly = v.code;
        if (rightOnly.empty() && v.left.empty() && !v.right.empty()) rightOnly = v.code;
        if (twoChildren.empty() && !v.left.empty() && !v.right.empty()) twoChildren = v.code;
    }
    CHECK(!leaf.empty() && !leftOnly.empty() && !rightOnly.empty() && !twoChildren.empty());
    if (leaf.empty() || leftOnly.empty() || rightOnly.empty() || twoChildren.empty()) return;

    std::cout << "delete cases on the file-order tree (each on a fresh tree; first such node breadth-first):\n";
    checkCaseOnRealTree(records, "D1 leaf          ", leaf, DeleteCase::Leaf);
    checkCaseOnRealTree(records, "D2 only left     ", leftOnly, DeleteCase::OnlyLeftChild);
    checkCaseOnRealTree(records, "D2 only right    ", rightOnly, DeleteCase::OnlyRightChild);
    checkCaseOnRealTree(records, "D3 two children  ", twoChildren, DeleteCase::TwoChildren);
    checkCaseOnRealTree(records, "root             ", nodes[0].code, DeleteCase::TwoChildren);
    // The sample session: remove BOS is case D2 with only a left child, BOR.
    checkCaseOnRealTree(records, "sample           ", "BOS", DeleteCase::OnlyLeftChild);
    AirportBST fresh;
    fresh.buildFromArray(records);
    RemoveResult bos = fresh.remove("BOS");
    CHECK(bos.describe() == "case D2: only a left child, BOR");
}

static void testDeleteEverything(const std::vector<Airport>& records) {
    AirportBST tree;
    tree.buildFromArray(records);
    std::vector<std::string> order = codesOf(records);
    std::mt19937 rng(2560);                                    // fixed seed: same order every run
    std::shuffle(order.begin(), order.end(), rng);

    int n = static_cast<int>(order.size());
    int counts[5] = {0, 0, 0, 0, 0};
    int rootDeletes = 0;
    bool allOk = true;
    for (int i = 0; i < n; i++) {
        RemoveResult r = tree.remove(order[i]);
        counts[static_cast<int>(r.kind)]++;
        if (r.wasRoot) rootDeletes++;
        if (r.kind == DeleteCase::NotFound || tree.size() != n - 1 - i || tree.find(order[i]).found) allOk = false;
        if (!tree.validate().ok) allOk = false;                // Task 10: validate after every delete
        if (i % 97 == 0) {                                     // full BST check now and then
            int bad = 0;
            std::vector<Airport> list = tree.inorder();
            if (static_cast<int>(list.size()) != n - 1 - i || !isSorted(list, bad)) allOk = false;
        }
    }
    CHECK(allOk);
    CHECK(tree.isEmpty() && tree.height() == -1);
    CHECK(counts[1] > 0 && counts[2] > 0 && counts[3] > 0 && counts[4] > 0 && rootDeletes > 0);
    std::cout << "deleted all " << n << " codes in shuffled order: D1 " << counts[1] << ", D2-left "
              << counts[2] << ", D2-right " << counts[3] << ", D3 " << counts[4] << ", root deletes "
              << rootDeletes << "; validate after every delete: " << (allOk ? "all OK" : "FAILED") << "\n";
}

int main() {
    testLectureExamples();
    testSmallCases();

    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load) && records.size() == 6072);
    testRealTree(records);
    testDeleteEverything(records);
    return finishTests();
}
