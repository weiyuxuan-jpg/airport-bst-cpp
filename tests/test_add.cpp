// Task 9 checks (project Task 3(c)): add the made-up airport QQQ, find it, remove it.
// QQQ = "Yuxuan Wei International Airport", Boston, United States, no ICAO, coordinates 0, 0
// (chosen by the author on 2026-10-21).
// On the file-order tree QQQ must go in as a leaf where the search for it fell off, be found
// with depth + 1 comparisons, and removing it (case D1) must give back exactly the old tree.
// Run from the project root.

#include <iostream>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "check.h"

int main() {
    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load) && records.size() == 6072);
    AirportBST tree;
    tree.buildFromArray(records);
    std::vector<std::string> levelsBefore = tree.levelOrder();
    int heightBefore = tree.height();
    double averageBefore = tree.averageDepth();

    // Before: not in the data, so the search falls off at a NULL child.
    FindResult missing = tree.find("QQQ");
    CHECK(!missing.found);
    const std::string parent = missing.path.empty() ? "" : missing.path.back();

    Airport qqq("Yuxuan Wei International Airport", "Boston", "United States", "QQQ", "", 0.0, 0.0);
    CHECK(tree.insert(qqq));
    CHECK(tree.size() == 6073);

    // Found, as a leaf hanging from the node where the search fell off.
    FindResult found = tree.find("QQQ");
    CHECK(found.found);
    CHECK(found.comparisons() == missing.comparisons() + 1);
    CHECK(found.path.size() >= 2 && found.path[found.path.size() - 2] == parent);
    CHECK(found.airport.summary() == "QQQ  Yuxuan Wei International Airport, Boston, United States");
    CHECK(found.airport.getIcao().empty());
    CHECK(found.airport.getLatitude() == 0.0 && found.airport.getLongitude() == 0.0);
    std::vector<Airport> list = tree.inorder();
    int bad = 0;
    CHECK(list.size() == 6073 && isSorted(list, bad));
    CHECK(tree.validate().ok && tree.validate().reached == 6073);   // Task 10 re-run

    // Adding it twice is a duplicate (I4) and changes nothing.
    CHECK(!tree.insert(Airport("Other", "X", "Y", "QQQ", "", 1.0, 1.0)));
    CHECK(tree.size() == 6073 && tree.find("QQQ").airport.getName() == "Yuxuan Wei International Airport");

    // Removed: case D1 (it is a leaf), not found any more, and the tree is exactly as before.
    RemoveResult r = tree.remove("QQQ");
    CHECK(r.kind == DeleteCase::Leaf && !r.wasRoot);
    CHECK(!tree.find("QQQ").found);
    CHECK(tree.size() == 6072);
    CHECK(tree.validate().ok && tree.validate().reached == 6072);   // Task 10 re-run
    CHECK(tree.levelOrder() == levelsBefore);
    CHECK(tree.height() == heightBefore && tree.averageDepth() == averageBefore);

    std::cout << "QQQ: search fell off after " << missing.comparisons() << " comparisons at "
              << parent << "; added at depth " << found.comparisons() - 1 << " (found with "
              << found.comparisons() << " comparisons); removed as " << r.describe()
              << "; tree identical to before: "
              << (tree.levelOrder() == levelsBefore ? "yes" : "no") << "\n";
    return finishTests();
}
