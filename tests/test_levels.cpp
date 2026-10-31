// Task 8 checks: level order and the top-levels view.
//   1. the lecture's tree: level order, top levels with "-" for empty positions, early stop,
//      the view from a chosen node, after each of the lecture's deletes
//   2. the file-order tree: level order lists every code once; inserting the codes in level
//      order rebuilds the same tree (the claim of docs/pseudocode.md, section 6)
//   3. the six delete cases of task 7, top three levels before and after (Task 3(b)), and
//      three levels around the deleted node's parent, where D1 / D2 changes show
// Run from the project root.

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "check.h"

using Rows = std::vector<std::string>;

static Airport ap(const std::string& code) {
    return Airport("Test " + code, "City", "Country", code, "", 0.0, 0.0);
}

static std::string joinRows(const Rows& rows) {
    std::string out;
    int n = static_cast<int>(rows.size());
    for (int i = 0; i < n; i++) out += (i > 0 ? " | " : "") + rows[i];
    return out;
}

// Row i must have exactly 2^i positions.
static bool widthsOk(const Rows& rows) {
    int n = static_cast<int>(rows.size());
    for (int i = 0; i < n; i++) {
        std::istringstream words(rows[i]);
        std::string w;
        int count = 0;
        while (words >> w) count++;
        if (count != (1 << i)) return false;
    }
    return true;
}

// rows with every occurrence of the word from replaced by to.
static Rows replaceCode(const Rows& rows, const std::string& from, const std::string& to) {
    Rows out;
    int n = static_cast<int>(rows.size());
    for (int i = 0; i < n; i++) {
        std::istringstream words(rows[i]);
        std::string w, row;
        while (words >> w) row += (row.empty() ? "" : " ") + (w == from ? to : w);
        out.push_back(row);
    }
    return out;
}

static void testLectureTree() {
    AirportBST t;
    t.buildFromArray({ap("K40"), ap("K20"), ap("K60"), ap("K10"), ap("K30"), ap("K50"), ap("K70")});
    CHECK(t.levelOrder() == (std::vector<std::string>{"K40", "K20", "K60", "K10", "K30", "K50", "K70"}));
    CHECK(t.topLevels(3) == (Rows{"K40", "K20 K60", "K10 K30 K50 K70"}));
    CHECK(t.topLevels(5) == t.topLevels(3));                // level 3 is empty: stops early
    CHECK(t.topLevels(1) == (Rows{"K40"}));
    CHECK(t.topLevels(0).empty());

    t.insert(ap("K35"));                                    // right child of K30
    Rows four = t.topLevels(4);
    CHECK(four.size() == 4 && four[3] == "- - - K35 - - - -");
    CHECK(widthsOk(four));
    CHECK(t.topLevels(3, "K20") == (Rows{"K20", "K10 K30", "- - - K35"}));   // view from K20
    CHECK(t.topLevels(3, "K35") == (Rows{"K35"}));         // a leaf: one row
    CHECK(t.topLevels(3, "K99").empty());                   // not in the tree

    // The lecture's deletes (Section 5.2), shown as top levels.
    t.remove("K10");
    CHECK(t.topLevels(4) == (Rows{"K40", "K20 K60", "- K30 K50 K70", "- - - K35 - - - -"}));
    t.remove("K30");
    CHECK(t.topLevels(4) == (Rows{"K40", "K20 K60", "- K35 K50 K70"}));
    t.remove("K40");
    CHECK(t.topLevels(4) == (Rows{"K50", "K20 K60", "- K35 - K70"}));
    CHECK(t.levelOrder() == (std::vector<std::string>{"K50", "K20", "K60", "K35", "K70"}));

    AirportBST empty;
    CHECK(empty.levelOrder().empty() && empty.topLevels(3).empty());
}

static void testRealLevelOrder(const std::vector<Airport>& records) {
    AirportBST tree;
    tree.buildFromArray(records);
    std::vector<std::string> order = tree.levelOrder();
    CHECK(static_cast<int>(order.size()) == tree.size());
    CHECK(!order.empty() && order[0] == "GKA");
    std::vector<std::string> sorted = order;
    std::sort(sorted.begin(), sorted.end());
    CHECK(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end());   // each code once

    // Level order agrees with the breadth-first node table, depth never decreases along it.
    std::vector<NodeInfo> nodes = tree.nodes();
    bool sameAsNodes = nodes.size() == order.size();
    bool depthsNonDecreasing = true;
    int n = static_cast<int>(nodes.size());
    for (int i = 0; i < n; i++) {
        if (nodes[i].code != order[i]) sameAsNodes = false;
        if (i > 0 && nodes[i].depth < nodes[i - 1].depth) depthsNonDecreasing = false;
    }
    CHECK(sameAsNodes && depthsNonDecreasing);

    // Inserting the codes in level order rebuilds the same tree: same level order, same height.
    std::map<std::string, Airport> byCode;
    int r = static_cast<int>(records.size());
    for (int i = 0; i < r; i++) byCode[records[i].getIata()] = records[i];
    AirportBST rebuilt;
    for (int i = 0; i < n; i++) rebuilt.insert(byCode[order[i]]);
    CHECK(rebuilt.levelOrder() == order);
    CHECK(rebuilt.height() == tree.height() && rebuilt.averageDepth() == tree.averageDepth());

    Rows top = tree.topLevels(3);
    CHECK(top == (Rows{"GKA", "AEY MAG", "AAE EGS HGU POM"}));
    std::cout << "file-order tree: level order has " << order.size() << " codes; top three levels: "
              << joinRows(top) << "\n";
}

struct DeleteCaseSpec {
    const char* label;
    const char* code;
    DeleteCase kind;
};

static void testDeleteCases(const std::vector<Airport>& records) {
    // The six cases picked in task 7 (PROGRESS.md, 7.2).
    const DeleteCaseSpec cases[] = {
        {"D1 leaf", "AAC", DeleteCase::Leaf},
        {"D2 only left", "GJT", DeleteCase::OnlyLeftChild},
        {"D2 only right", "AAA", DeleteCase::OnlyRightChild},
        {"D3 two children", "AEY", DeleteCase::TwoChildren},
        {"root (D3)", "GKA", DeleteCase::TwoChildren},
        {"sample (D2)", "BOS", DeleteCase::OnlyLeftChild},
    };
    std::cout << "top three levels before / after each delete (fresh file-order tree each time):\n";
    int count = static_cast<int>(sizeof(cases) / sizeof(cases[0]));
    for (int c = 0; c < count; c++) {
        const std::string code = cases[c].code;
        AirportBST tree;
        tree.buildFromArray(records);

        // Depth and parent of the node to delete.
        std::vector<NodeInfo> nodes = tree.nodes();
        int depth = -1;
        std::string parent = code;                          // the root has no parent: view from itself
        int n = static_cast<int>(nodes.size());
        for (int i = 0; i < n; i++) {
            if (nodes[i].code == code) depth = nodes[i].depth;
            if (nodes[i].left == code || nodes[i].right == code) parent = nodes[i].code;
        }

        Rows topBefore = tree.topLevels(3);
        Rows localBefore = tree.topLevels(3, parent);
        RemoveResult r = tree.remove(code);
        CHECK(r.kind == cases[c].kind);
        std::string viewAfter = r.wasRoot ? r.other : parent;   // root case: the root now holds the successor
        Rows topAfter = tree.topLevels(3);
        Rows localAfter = tree.topLevels(3, viewAfter);
        CHECK(widthsOk(topBefore) && widthsOk(topAfter) && widthsOk(localBefore) && widthsOk(localAfter));

        if (depth >= 3) {
            // Neither the node nor anything that moves up is in levels 0-2.
            CHECK(topAfter == topBefore);
            CHECK(localAfter != localBefore);
        } else {
            // D3 within the top three levels: the successor's code takes the deleted code's
            // place (the successor itself came from deeper down).
            CHECK(r.kind == DeleteCase::TwoChildren);
            CHECK(joinRows(topBefore).find(r.other) == std::string::npos);
            CHECK(topAfter == replaceCode(topBefore, code, r.other));
        }

        std::cout << "  " << cases[c].label << " " << code << " (depth " << depth << "): " << r.describe() << "\n"
                  << "    top 3 before: " << joinRows(topBefore) << "\n"
                  << "    top 3 after:  " << joinRows(topAfter)
                  << (topAfter == topBefore ? "   (unchanged)" : "") << "\n";
        if (depth >= 3) {
            std::cout << "    around parent " << parent << " before: " << joinRows(localBefore) << "\n"
                      << "    around parent " << parent << " after:  " << joinRows(localAfter) << "\n";
        }
    }
}

int main() {
    testLectureTree();

    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load) && records.size() == 6072);
    testRealLevelOrder(records);
    testDeleteCases(records);
    return finishTests();
}
