#include "AirportBST.h"

#include <queue>
#include <utility>
#include <vector>

AirportBST::AirportBST() : root(nullptr), count(0) {}

AirportBST::~AirportBST() {
    clear();
}

// Iterative with an explicit stack: a tree built from sorted keys is a chain of
// 6,071 nodes, and a recursive delete would need one call frame per level.
void AirportBST::deleteSubtree(Node* top) {
    std::vector<Node*> stack;
    if (top != nullptr) stack.push_back(top);
    while (!stack.empty()) {
        Node* node = stack.back();
        stack.pop_back();
        if (node->left != nullptr) stack.push_back(node->left);
        if (node->right != nullptr) stack.push_back(node->right);
        delete node;
    }
}

void AirportBST::clear() {
    deleteSubtree(root);
    root = nullptr;
    count = 0;
}

// Iterative, as the project note asks: sorted input builds a 6,071-level chain.
bool AirportBST::insert(const Airport& airport) {
    const std::string& key = airport.getIata();                   // step 1
    if (root == nullptr) {                                        // step 2: I1 empty tree
        root = new Node(airport);
        count++;
        return true;
    }
    Node* cur = root;                                             // step 3
    while (true) {                                                // step 4
        if (key < cur->data.getIata()) {                          // step 5: I2 go left
            if (cur->left == nullptr) {                           // step 6
                cur->left = new Node(airport);
                count++;
                return true;
            }
            cur = cur->left;                                      // step 7
        } else if (key > cur->data.getIata()) {                   // step 8: I3 go right
            if (cur->right == nullptr) {                          // step 9
                cur->right = new Node(airport);
                count++;
                return true;
            }
            cur = cur->right;                                     // step 10
        } else {
            return false;                                         // step 11: I4 duplicate
        }
    }
}

BuildReport AirportBST::buildFromArray(const std::vector<Airport>& records) {
    BuildReport report;                                           // step 1
    int n = static_cast<int>(records.size());
    for (int i = 0; i < n; i++) {                                 // step 2
        if (insert(records[i])) report.inserted++;                // step 3
        else report.duplicates.push_back(records[i].getIata());   // step 4: I4
    }
    return report;                                                // step 5
}

void AirportBST::measureDepths(int& treeHeight, long long& totalDepth) const {
    treeHeight = -1;
    totalDepth = 0;
    if (root == nullptr) return;                                  // step 1

    std::queue<std::pair<const Node*, int>> q;                    // (node, depth)
    q.push(std::make_pair(root, 0));                              // step 2
    treeHeight = 0;                                               // step 3
    while (!q.empty()) {                                          // step 4
        const Node* node = q.front().first;                       // step 5
        int depth = q.front().second;
        q.pop();
        totalDepth += depth;                                      // step 6
        if (depth > treeHeight) treeHeight = depth;               // step 7
        if (node->left != nullptr) q.push(std::make_pair(node->left, depth + 1));    // step 8
        if (node->right != nullptr) q.push(std::make_pair(node->right, depth + 1));  // step 9
    }
}

int AirportBST::height() const {
    int treeHeight = 0;
    long long totalDepth = 0;
    measureDepths(treeHeight, totalDepth);
    return treeHeight;
}

double AirportBST::averageDepth() const {
    if (count == 0) return 0.0;
    int treeHeight = 0;
    long long totalDepth = 0;
    measureDepths(treeHeight, totalDepth);
    return static_cast<double>(totalDepth) / count;               // step 10
}

std::vector<Airport> AirportBST::inorder() const {
    std::vector<Airport> out;                                     // step 1
    out.reserve(count);
    std::vector<const Node*> stack;
    const Node* cur = root;
    while (cur != nullptr || !stack.empty()) {                    // step 2
        while (cur != nullptr) {                                  // step 3: go left
            stack.push_back(cur);
            cur = cur->left;
        }
        cur = stack.back();                                       // step 4
        stack.pop_back();
        out.push_back(cur->data);                                 // step 5: visit
        cur = cur->right;                                         // step 6
    }
    return out;                                                   // step 7
}

FindResult AirportBST::find(const std::string& code) const {
    FindResult result;                                            // step 1
    const Node* cur = root;
    while (cur != nullptr) {                                      // step 2
        result.path.push_back(cur->data.getIata());               // step 3
        int c = code.compare(cur->data.getIata());                // the one comparison
        if (c == 0) {                                             // step 4: S2 found
            result.found = true;
            result.airport = cur->data;
            return result;
        }
        if (c < 0) cur = cur->left;                               // step 5: S3
        else cur = cur->right;                                    // step 6: S4
    }
    return result;                                                // step 7: S1 not found
}

bool AirportBST::minimum(Airport& out) const {
    if (root == nullptr) return false;                            // step 1
    const Node* cur = root;                                       // step 2
    while (cur->left != nullptr) cur = cur->left;                 // step 3: keep going left
    out = cur->data;                                              // step 4
    return true;
}

bool AirportBST::maximum(Airport& out) const {
    if (root == nullptr) return false;
    const Node* cur = root;
    while (cur->right != nullptr) cur = cur->right;               // keep going right
    out = cur->data;
    return true;
}

std::vector<NodeInfo> AirportBST::nodes() const {
    std::vector<NodeInfo> out;
    if (root == nullptr) return out;
    std::queue<std::pair<const Node*, int>> q;
    q.push(std::make_pair(root, 0));
    while (!q.empty()) {
        const Node* node = q.front().first;
        NodeInfo info;
        info.code = node->data.getIata();
        info.depth = q.front().second;
        q.pop();
        if (node->left != nullptr) {
            info.left = node->left->data.getIata();
            q.push(std::make_pair(node->left, info.depth + 1));
        }
        if (node->right != nullptr) {
            info.right = node->right->data.getIata();
            q.push(std::make_pair(node->right, info.depth + 1));
        }
        out.push_back(info);
    }
    return out;
}

// Iterative with a parent pointer, so deleting the root is an explicit step (16) and the
// case can be reported. Same three cases as the lecture's recursive DELETE.
RemoveResult AirportBST::remove(const std::string& code) {
    RemoveResult result;
    Node* parent = nullptr;                                       // step 1
    Node* cur = root;
    while (cur != nullptr && cur->data.getIata() != code) {       // step 2: search,
        parent = cur;                                             // step 3: remembering the parent
        if (code < cur->data.getIata()) cur = cur->left;          // step 4
        else cur = cur->right;
    }
    if (cur == nullptr) return result;                            // step 5: not in the tree
    result.wasRoot = (parent == nullptr);

    if (cur->left == nullptr && cur->right == nullptr) {          // step 6: D1 leaf
        result.kind = DeleteCase::Leaf;
    } else if (cur->right == nullptr) {                           // step 7: D2 left child only
        result.kind = DeleteCase::OnlyLeftChild;
        result.other = cur->left->data.getIata();
    } else if (cur->left == nullptr) {                            // step 8: D2 right child only
        result.kind = DeleteCase::OnlyRightChild;
        result.other = cur->right->data.getIata();
    } else {                                                      // step 9: D3 two children
        Node* sParent = cur;                                      // step 10
        Node* s = cur->right;
        while (s->left != nullptr) {                              // step 11: s = inorder successor
            sParent = s;
            s = s->left;
        }
        result.kind = DeleteCase::TwoChildren;                    // step 12
        result.other = s->data.getIata();
        cur->data = s->data;                                      // step 13: copy its record
        cur = s;                                                  // step 14: now delete s,
        parent = sParent;                                         //          which is D1 or D2
    }

    Node* child = (cur->left != nullptr) ? cur->left : cur->right;   // step 15: NULL for D1
    if (parent == nullptr) root = child;                          // step 16: deleting the root
    else if (parent->left == cur) parent->left = child;           // step 17: compare pointers,
    else parent->right = child;                                   // step 18: not keys
    delete cur;                                                   // step 19
    count--;
    return result;                                                // step 20
}

std::vector<std::string> AirportBST::levelOrder() const {
    std::vector<std::string> out;                                 // step 1
    if (root == nullptr) return out;                              // step 2
    std::queue<const Node*> q;                                    // step 3
    q.push(root);
    while (!q.empty()) {                                          // step 4
        const Node* node = q.front();                             // step 5
        q.pop();
        out.push_back(node->data.getIata());
        if (node->left != nullptr) q.push(node->left);            // step 6
        if (node->right != nullptr) q.push(node->right);          // step 7
    }
    return out;                                                   // step 8
}

const AirportBST::Node* AirportBST::findNode(const std::string& code) const {
    const Node* cur = root;
    while (cur != nullptr) {
        int c = code.compare(cur->data.getIata());
        if (c == 0) return cur;
        cur = (c < 0) ? cur->left : cur->right;
    }
    return nullptr;
}

std::vector<std::string> AirportBST::topLevels(int k, const std::string& fromCode) const {
    std::vector<std::string> rows;
    const Node* start = root;
    if (!fromCode.empty()) {
        start = findNode(fromCode);
        if (start == nullptr) return rows;                        // not in the tree
    }
    std::queue<const Node*> q;                                    // step 1: may hold nullptr
    q.push(start);
    for (int level = 0; level < k; level++) {                     // step 2
        int m = static_cast<int>(q.size());                       // step 3: 2^level positions
        std::string row;
        bool anyNode = false;
        for (int i = 0; i < m; i++) {                             // step 4
            const Node* node = q.front();                         // step 5
            q.pop();
            if (i > 0) row += " ";
            if (node == nullptr) {                                // step 6: empty position
                row += "-";
                q.push(nullptr);
                q.push(nullptr);
            } else {                                              // step 7
                row += node->data.getIata();
                anyNode = true;
                q.push(node->left);
                q.push(node->right);
            }
        }
        if (!anyNode) break;                                      // nothing at this level or below
        rows.push_back(row);                                      // step 8
    }
    return rows;
}

bool AirportBST::isBst(const Node* node, const std::string* lo, const std::string* hi,
                       ValidateResult& result) const {
    if (node == nullptr) return true;                             // step 1
    result.reached++;                                             // step 2
    const std::string& key = node->data.getIata();
    if ((lo != nullptr && key <= *lo) || (hi != nullptr && key >= *hi)) {   // step 3
        result.badCode = key;                                     // step 4: first violation
        result.low = (lo != nullptr) ? *lo : "";
        result.high = (hi != nullptr) ? *hi : "";
        return false;
    }
    return isBst(node->left, lo, &key, result) &&                 // step 5: left keys < key,
           isBst(node->right, &key, hi, result);                  //         right keys > key
}

ValidateResult AirportBST::validate() const {
    ValidateResult result;
    result.count = count;
    bool inRange = isBst(root, nullptr, nullptr, result);         // step 1
    result.ok = inRange && result.reached == count;               // step 2
    return result;                                                // step 3
}

std::vector<Airport> AirportBST::preorder() const {
    std::vector<Airport> out;                                     // step 1
    out.reserve(count);
    if (root == nullptr) return out;                              // step 2
    std::vector<const Node*> stack;                               // step 3
    stack.push_back(root);
    while (!stack.empty()) {                                      // step 4
        const Node* node = stack.back();                          // step 5: visit first
        stack.pop_back();
        out.push_back(node->data);
        if (node->right != nullptr) stack.push_back(node->right); // step 6: right first,
        if (node->left != nullptr) stack.push_back(node->left);   // step 7: so left pops first
    }
    return out;                                                   // step 8
}

AirportBST::Node* AirportBST::buildPreorderRange(const std::vector<Airport>& list, int& idx,
                                                 const std::string* lo, const std::string* hi) {
    if (idx == static_cast<int>(list.size())) return nullptr;     // step 1: nothing left
    const std::string& key = list[idx].getIata();                 // step 2
    if ((lo != nullptr && key <= *lo) || (hi != nullptr && key >= *hi)) {
        return nullptr;                                           // step 3: belongs higher up
    }
    Node* node = new Node(list[idx]);                             // step 4
    idx++;
    node->left = buildPreorderRange(list, idx, lo, &key);         // step 5: keys in (lo, key)
    node->right = buildPreorderRange(list, idx, &key, hi);        // step 6: keys in (key, hi)
    return node;                                                  // step 7
}

bool AirportBST::buildFromPreorder(const std::vector<Airport>& list, int& stoppedAt) {
    int idx = 0;
    Node* newRoot = buildPreorderRange(list, idx, nullptr, nullptr);   // LOAD-TREE step 7
    int n = static_cast<int>(list.size());
    if (idx != n) {                                               // step 8: not a BST preorder
        deleteSubtree(newRoot);
        stoppedAt = idx;
        return false;                                             // the old tree is kept
    }
    clear();                                                      // step 9
    root = newRoot;
    count = n;
    stoppedAt = -1;
    return true;
}

void AirportBST::rangeVisit(const Node* node, const std::string& from, const std::string& to,
                            RangeResult& result) {
    if (node == nullptr) return;                                  // step 1
    result.visited++;                                             // step 2
    const std::string& key = node->data.getIata();
    if (from < key) rangeVisit(node->left, from, to, result);     // step 3: left may hold codes >= from
    if (from <= key && key <= to) result.airports.push_back(node->data);   // step 4: in order
    if (key < to) rangeVisit(node->right, from, to, result);      // step 5: right may hold codes <= to
}

RangeResult AirportBST::range(const std::string& from, const std::string& to) const {
    RangeResult result;
    if (from > to) return result;                                 // nothing can be in [from, to]
    rangeVisit(root, from, to, result);
    return result;
}

AirportBST::Node* AirportBST::buildBalancedRange(const std::vector<Airport>& a, int lo, int hi) {
    if (lo > hi) return nullptr;                                  // step 1: empty range
    int mid = lo + (hi - lo) / 2;                                 // step 2: floor((lo + hi) / 2)
    Node* node = new Node(a[mid]);                                // step 3: middle is the root
    node->left = buildBalancedRange(a, lo, mid - 1);              // step 4: left half
    node->right = buildBalancedRange(a, mid + 1, hi);             // step 5: right half
    return node;                                                  // step 6
}

bool AirportBST::buildBalanced(const std::vector<Airport>& sortedList) {
    int bad = -1;
    if (!isSorted(sortedList, bad)) return false;                 // step 1: must be a BST order
    int n = static_cast<int>(sortedList.size());
    clear();                                                      // step 2
    root = buildBalancedRange(sortedList, 0, n - 1);
    count = n;
    return true;                                                  // step 3
}

void AirportBST::rebalance() {
    std::vector<Airport> a = inorder();                           // step 1: sorted (P1)
    buildBalanced(a);                                             // step 2: always sorted here
}

bool AirportBST::countFind(const std::string& code, int& comparisons) const {
    comparisons = 0;
    const Node* cur = root;
    while (cur != nullptr) {
        comparisons++;
        int c = code.compare(cur->data.getIata());
        if (c == 0) return true;
        cur = (c < 0) ? cur->left : cur->right;
    }
    return false;
}

std::string ValidateResult::describe() const {
    if (ok) return "OK";
    if (!badCode.empty()) {
        return "FAILED (" + badCode + " is not in (" + (low.empty() ? "-inf" : low) + ", " +
               (high.empty() ? "+inf" : high) + "))";
    }
    return "FAILED (" + std::to_string(reached) + " nodes reached, count is " +
           std::to_string(count) + ")";
}

std::string RemoveResult::describe() const {
    switch (kind) {
        case DeleteCase::Leaf: return "case D1: leaf";
        case DeleteCase::OnlyLeftChild: return "case D2: only a left child, " + other;
        case DeleteCase::OnlyRightChild: return "case D2: only a right child, " + other;
        case DeleteCase::TwoChildren: return "case D3: two children, successor " + other;
        case DeleteCase::NotFound: break;
    }
    return "not found";
}

bool isSorted(const std::vector<Airport>& list, int& badIndex) {
    int n = static_cast<int>(list.size());
    for (int i = 0; i < n - 1; i++) {                             // step 1
        if (list[i].getIata() >= list[i + 1].getIata()) {         // step 2
            badIndex = i;
            return false;
        }
    }
    badIndex = -1;
    return true;                                                  // step 3
}
