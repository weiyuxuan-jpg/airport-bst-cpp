// Task 5 checks: insert (I1-I4), build in file order, height, average depth, inorder.
//   1. the lecture's running example, with codes K10 ... K70 standing for keys 10 ... 70
//   2. edge cases: empty tree, one node, duplicates, clear and reuse
//   3. the real data in file order (expected: 6,072 inserted, 0 duplicates, height 177,
//      average depth 18.3, inorder strictly increasing)
//   4. the real data in sorted order: a 6,071-level chain that insert, inorder and the
//      destructor must handle without recursion
// Run from the project root.

#include <algorithm>
#include <iostream>
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

static void testLectureExample() {
    // keys = {40, 20, 60, 10, 30, 50, 70}, inserted in this order (Lecture 07, Figure 2)
    std::vector<Airport> data = {ap("K40"), ap("K20"), ap("K60"), ap("K10"),
                                 ap("K30"), ap("K50"), ap("K70")};
    AirportBST tree;
    BuildReport build = tree.buildFromArray(data);
    CHECK(build.inserted == 7);
    CHECK(build.duplicates.empty());
    CHECK(tree.size() == 7);
    CHECK(tree.height() == 2);                                  // lecture: height 2
    CHECK(tree.averageDepth() == 10.0 / 7.0);                   // depths 0+1+1+2+2+2+2
    CHECK(codesOf(tree.inorder()) ==
          (std::vector<std::string>{"K10", "K20", "K30", "K40", "K50", "K60", "K70"}));

    CHECK(tree.validate().ok);
    CHECK(tree.insert(ap("K35")));                              // lecture Figure 4
    CHECK(tree.height() == 3);
    CHECK(tree.validate().ok);
    int bad = 0;
    CHECK(isSorted(tree.inorder(), bad) && bad == -1);

    // Sorted insertion order gives a chain of height n - 1 = 6 (lecture Figure 5).
    std::vector<Airport> sorted = data;
    std::sort(sorted.begin(), sorted.end(),
              [](const Airport& a, const Airport& b) { return a.getIata() < b.getIata(); });
    AirportBST chain;
    chain.buildFromArray(sorted);
    CHECK(chain.height() == 6);
    CHECK(chain.averageDepth() == 3.0);                         // (0+1+...+6) / 7
}

static void testEdgeCases() {
    AirportBST empty;
    CHECK(empty.height() == -1);
    CHECK(empty.averageDepth() == 0.0);
    CHECK(empty.inorder().empty());
    int bad = 0;
    CHECK(isSorted(empty.inorder(), bad) && bad == -1);

    AirportBST one;
    CHECK(one.insert(ap("BOS")));                               // I1: becomes the root
    CHECK(one.size() == 1);
    CHECK(one.height() == 0);
    CHECK(one.averageDepth() == 0.0);

    // I4: a duplicate is rejected and leaves the tree unchanged.
    CHECK(!one.insert(Airport("Another Boston", "X", "Y", "BOS", "", 1.0, 1.0)));
    CHECK(one.size() == 1);
    std::vector<Airport> list = one.inorder();
    CHECK(list.size() == 1 && list[0].getName() == "Test BOS");   // the first record stays

    AirportBST tree;
    BuildReport build = tree.buildFromArray({ap("MMM"), ap("AAA"), ap("MMM"), ap("ZZZ"), ap("AAA")});
    CHECK(build.inserted == 3);
    CHECK(build.duplicates == (std::vector<std::string>{"MMM", "AAA"}));
    CHECK(tree.size() == 3);

    tree.clear();
    CHECK(tree.size() == 0 && tree.isEmpty() && tree.height() == -1);
    CHECK(tree.insert(ap("QQQ")) && tree.size() == 1);          // usable again after clear

    // isSorted must find an unsorted pair and a repeated code.
    CHECK(!isSorted({ap("AAA"), ap("CCC"), ap("BBB")}, bad) && bad == 1);
    CHECK(!isSorted({ap("AAA"), ap("AAA")}, bad) && bad == 0);
}

static void testRealDataFileOrder(const std::vector<Airport>& records) {
    AirportBST tree;
    BuildReport build = tree.buildFromArray(records);
    CHECK(build.inserted == 6072);
    CHECK(build.duplicates.empty());
    CHECK(tree.size() == 6072);

    int h = tree.height();
    double avg = tree.averageDepth();
    CHECK(h == 177);
    CHECK(avg >= 18.25 && avg < 18.35);                         // prints as 18.3

    std::vector<Airport> list = tree.inorder();
    int bad = 0;
    CHECK(list.size() == 6072);
    CHECK(isSorted(list, bad) && bad == -1);

    // Inorder must list exactly the loaded codes, sorted (nothing lost or repeated).
    std::vector<std::string> expected = codesOf(records);
    std::sort(expected.begin(), expected.end());
    CHECK(codesOf(list) == expected);
    CHECK(!list.empty() && list.front().getIata() == "AAA" && list.back().getIata() == "ZZV");

    std::cout << "file order: " << build.inserted << " inserted, " << build.duplicates.size()
              << " duplicates, height " << h << ", average depth " << avg << ", inorder "
              << list.size() << " codes, sorted = " << (bad == -1 ? "yes" : "no") << " ("
              << list.front().getIata() << " ... " << list.back().getIata() << ")\n";
}

static void testRealDataSortedOrder(const std::vector<Airport>& records) {
    std::vector<Airport> sorted = records;
    std::sort(sorted.begin(), sorted.end(),
              [](const Airport& a, const Airport& b) { return a.getIata() < b.getIata(); });
    AirportBST chain;
    BuildReport build = chain.buildFromArray(sorted);
    CHECK(build.inserted == 6072);
    CHECK(chain.height() == 6071);                              // n - 1
    CHECK(chain.averageDepth() == 6071.0 / 2.0);                // (0+1+...+6071) / 6072
    int bad = 0;
    std::vector<Airport> list = chain.inorder();
    CHECK(list.size() == 6072 && isSorted(list, bad));
    std::cout << "sorted order: height " << chain.height() << ", average depth "
              << chain.averageDepth() << " (the destructor now frees a 6,071-level chain)\n";
}

// Task 10, re-run on the insert tests: validate after every single insert (project Task 4(c)),
// in file order and in sorted order (the sorted chain makes validate recurse up to 6,072 deep).
static void testValidateAfterEveryInsert(const std::vector<Airport>& records) {
    std::vector<Airport> sorted = records;
    std::sort(sorted.begin(), sorted.end(),
              [](const Airport& a, const Airport& b) { return a.getIata() < b.getIata(); });
    const std::vector<Airport>* orders[2] = {&records, &sorted};
    const char* names[2] = {"file order", "sorted order"};
    for (int k = 0; k < 2; k++) {
        AirportBST tree;
        bool allValid = true;
        int n = static_cast<int>(orders[k]->size());
        for (int i = 0; i < n; i++) {
            tree.insert((*orders[k])[i]);
            ValidateResult v = tree.validate();
            if (!v.ok || v.reached != i + 1) allValid = false;
        }
        CHECK(allValid);
        std::cout << "validate after each of the " << n << " inserts (" << names[k] << "): "
                  << (allValid ? "all OK" : "FAILED") << "\n";
    }
}

int main() {
    testLectureExample();
    testEdgeCases();

    std::vector<Airport> records;
    LoadReport report;
    CHECK(loadAirports("data/airports.dat", records, report));
    CHECK(records.size() == 6072);
    testRealDataFileOrder(records);
    testRealDataSortedOrder(records);
    testValidateAfterEveryInsert(records);
    return finishTests();
}
