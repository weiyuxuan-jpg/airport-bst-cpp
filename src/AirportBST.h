#ifndef AIRPORT_BST_H
#define AIRPORT_BST_H

#include <string>
#include <vector>

#include "Airport.h"

// Result of BUILD-FROM-ARRAY.
struct BuildReport {
    int inserted = 0;                     // new nodes added
    std::vector<std::string> duplicates;  // codes rejected by rule I4, in input order
};

// Result of FIND.
struct FindResult {
    bool found = false;
    Airport airport;                      // a copy of the record, when found
    std::vector<std::string> path;        // every code compared, from the root down

    // One three-way comparison per node on the path.
    int comparisons() const { return static_cast<int>(path.size()); }
};

// Which of the lecture's delete cases REMOVE used.
enum class DeleteCase {
    NotFound,
    Leaf,             // D1
    OnlyLeftChild,    // D2
    OnlyRightChild,   // D2
    TwoChildren       // D3 (inorder successor)
};

// Result of REMOVE.
struct RemoveResult {
    DeleteCase kind = DeleteCase::NotFound;
    std::string other;     // D2: the child that moves up; D3: the successor that was copied
    bool wasRoot = false;  // the deleted code was in the root

    // "case D2: only a left child, BOR", as in the sample session.
    std::string describe() const;
};

// Result of VALIDATE.
struct ValidateResult {
    bool ok = true;
    int reached = 0;       // nodes visited by IS-BST (all of them when ok)
    int count = 0;         // the tree's own node count
    std::string badCode;   // first node outside its allowed range, "" if none
    std::string low;       // that range: (low, high), "" = no bound
    std::string high;

    // "OK", "FAILED (K45 is not in (K20, K40))" or "FAILED (6072 nodes reached, count is 6073)".
    std::string describe() const;
};

// Result of RANGE.
struct RangeResult {
    std::vector<Airport> airports;   // every record with from <= code <= to, in code order
    int visited = 0;                 // non-NULL nodes the search looked at
};

// One node's place in the tree (a read-only copy, for tests and diagnostics).
struct NodeInfo {
    std::string code;
    int depth = 0;
    std::string left;                     // code of the left child, "" if none
    std::string right;                    // code of the right child, "" if none
};

// Binary search tree of Airport records keyed by IATA code (std::string compare).
// Owns every node: nodes are created with new and freed in clear() / the destructor.
//
// Conventions used by every operation:
//   - height counts edges: empty tree = -1, a single node = 0
//   - depth of the root = 0
//   - keys are unique; inserting an existing code is a duplicate and is rejected
class AirportBST {
public:
    AirportBST();
    ~AirportBST();

    // A tree owns its nodes, so a shallow copy would free them twice.
    AirportBST(const AirportBST&) = delete;
    AirportBST& operator=(const AirportBST&) = delete;

    int size() const { return count; }
    bool isEmpty() const { return root == nullptr; }

    // Deletes every node; the tree becomes empty.
    void clear();

    // INSERT (rules I1-I4, iterative): adds the airport as a new leaf.
    // Returns false, and changes nothing, if its code is already in the tree.
    bool insert(const Airport& airport);

    // BUILD-FROM-ARRAY (method B1): inserts the records in the given order into this tree.
    BuildReport buildFromArray(const std::vector<Airport>& records);

    // Height in edges (empty tree -1) and mean node depth (root 0; empty tree 0.0).
    int height() const;
    double averageDepth() const;

    // INORDER with an explicit stack: every record in increasing code order (P1).
    std::vector<Airport> inorder() const;

    // FIND (rules S1-S4, iterative): the search path and, if present, the record.
    FindResult find(const std::string& code) const;

    // MINIMUM / MAXIMUM (P2): leftmost / rightmost record. False if the tree is empty.
    bool minimum(Airport& out) const;
    bool maximum(Airport& out) const;

    // Every node with its depth and children, breadth-first (root first).
    std::vector<NodeInfo> nodes() const;

    // REMOVE (cases D1, D2, D3 with the inorder successor; iterative). Deleting the
    // root updates root. Returns which case was used, or NotFound.
    RemoveResult remove(const std::string& code);

    // LEVEL-ORDER: every code, level by level from the root, left to right.
    std::vector<std::string> levelOrder() const;

    // TOP-LEVELS: up to k levels, one string per level, "-" for an empty position
    // (level d has 2^d positions). Starts at the root, or at the node holding fromCode.
    // Stops early at a level with no nodes; returns no rows if fromCode is not in the tree.
    std::vector<std::string> topLevels(int k, const std::string& fromCode = "") const;

    // VALIDATE: every node lies strictly inside the (lo, hi) range set by all its ancestors
    // (lecture P5), and the number of nodes reached equals count.
    ValidateResult validate() const;

    // PREORDER with an explicit stack: every record, node before its subtrees (for SAVE).
    std::vector<Airport> preorder() const;

    // BUILD-FROM-PREORDER (method B3, bounds (lo, hi), O(n)): replaces this tree with the
    // BST whose preorder is list. If list is not the preorder of a BST (out of order, or a
    // duplicate), nothing changes, false is returned and stoppedAt is the index of the
    // first record that did not fit.
    bool buildFromPreorder(const std::vector<Airport>& list, int& stoppedAt);

    // RANGE: every airport with from <= code <= to (both ends included; they need not be
    // in the tree), entering a subtree only if it can hold such a code. Empty if from > to.
    RangeResult range(const std::string& from, const std::string& to) const;

    // BUILD-BALANCED-TREE (method B2, middle rule, O(n)): replaces this tree with a tree of
    // height floor(log2 n) built from sortedList. Returns false, and changes nothing, if the
    // list is not strictly increasing by code (the result would not be a BST).
    bool buildBalanced(const std::vector<Airport>& sortedList);

    // REBALANCE: inorder (explicit stack), then buildBalanced. Same airports, height floor(log2 n).
    void rebalance();

    // COUNT-FIND: FIND without the path, for timing. Sets comparisons; true if found.
    bool countFind(const std::string& code, int& comparisons) const;

private:
    // Lets tests/test_validate.cpp build broken trees to show that validate catches them.
    // It is defined only there; the program itself never uses it.
    friend struct AirportBSTTestAccess;

    // Tree node: one Airport plus links to the two subtrees.
    // Private so that code outside the class cannot relink nodes and break the BST property.
    struct Node {
        Airport data;
        Node* left;
        Node* right;

        explicit Node(const Airport& airport) : data(airport), left(nullptr), right(nullptr) {}
    };

    Node* root;
    int count;   // number of nodes in the tree

    // HEIGHT-AND-DEPTH: one breadth-first pass gives the height and the sum of all depths.
    void measureDepths(int& treeHeight, long long& totalDepth) const;

    // The node holding code, or nullptr (S1-S4 without the path).
    const Node* findNode(const std::string& code) const;

    // IS-BST(node, lo, hi); lo / hi are nullptr for "no bound". Recursive, depth <= h + 1.
    bool isBst(const Node* node, const std::string* lo, const std::string* hi,
               ValidateResult& result) const;

    // B3 step for one subtree: takes list[idx], list[idx + 1], ... while they fit in (lo, hi).
    // Recursive, depth <= h + 1 of the tree being built.
    static Node* buildPreorderRange(const std::vector<Airport>& list, int& idx,
                                    const std::string* lo, const std::string* hi);

    // Frees every node of the subtree at top (explicit stack).
    static void deleteSubtree(Node* top);

    // RANGE(node, from, to) for one subtree. Recursive, depth <= h + 1.
    static void rangeVisit(const Node* node, const std::string& from, const std::string& to,
                           RangeResult& result);

    // BUILD-BALANCED(a, lo, hi). Recursive, depth <= floor(log2 n) + 1.
    static Node* buildBalancedRange(const std::vector<Airport>& a, int lo, int hi);
};

// IS-SORTED: true if the codes are strictly increasing. Otherwise badIndex is the first i
// with list[i] >= list[i + 1]; it is -1 when the list is sorted.
bool isSorted(const std::vector<Airport>& list, int& badIndex);

#endif
