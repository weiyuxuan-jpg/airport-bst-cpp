// Task 11 checks (project Task 4(b)): save in preorder, load rebuilds the same tree.
//   1. preorder and the B3 rebuild on the lecture's trees, including lists that are not the
//      preorder of any BST (out of order, duplicate): rejected, the old tree kept
//   2. the saved-line format and the file detection used by load
//   3. round trips on real trees: file order, after removing BOS, with QQQ added, and the
//      6,071-level sorted chain (B3 recursion 6,072 deep). After each: same height, same
//      average depth, same full level order, same records field by field, validate OK, and
//      saving the rebuilt tree writes a byte-identical file
//   4. damaged saved files: rejected with the right line, the old tree kept
// Run from the project root.

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "check.h"

static Airport ap(const std::string& code) {
    return Airport("Test " + code, "City", "Country", code, "", 0.0, 0.0);
}

static std::vector<Airport> aps(const std::vector<std::string>& codes) {
    std::vector<Airport> out;
    int n = static_cast<int>(codes.size());
    for (int i = 0; i < n; i++) out.push_back(ap(codes[i]));
    return out;
}

static std::vector<std::string> codesOf(const std::vector<Airport>& list) {
    std::vector<std::string> codes;
    int n = static_cast<int>(list.size());
    for (int i = 0; i < n; i++) codes.push_back(list[i].getIata());
    return codes;
}

static bool sameRecords(const std::vector<Airport>& a, const std::vector<Airport>& b) {
    if (a.size() != b.size()) return false;
    int n = static_cast<int>(a.size());
    for (int i = 0; i < n; i++) {
        if (a[i].getIata() != b[i].getIata() || a[i].getName() != b[i].getName() ||
            a[i].getCity() != b[i].getCity() || a[i].getCountry() != b[i].getCountry() ||
            a[i].getIcao() != b[i].getIcao() || a[i].getLatitude() != b[i].getLatitude() ||
            a[i].getLongitude() != b[i].getLongitude()) {
            return false;
        }
    }
    return true;
}

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

static std::vector<std::string> readLines(const std::string& path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

static void writeLines(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream out(path);
    int n = static_cast<int>(lines.size());
    for (int i = 0; i < n; i++) out << lines[i] << "\n";
}

static void testPreorderAndB3() {
    AirportBST t;
    t.buildFromArray(aps({"K40", "K20", "K60", "K10", "K30", "K50", "K70"}));
    CHECK(codesOf(t.preorder()) == (std::vector<std::string>{"K40", "K20", "K10", "K30", "K60", "K50", "K70"}));
    t.insert(ap("K35"));                                    // lecture Section 5.2 start
    CHECK(codesOf(t.preorder()) == (std::vector<std::string>{"K40", "K20", "K10", "K30", "K35", "K60", "K50", "K70"}));

    // B3 on the tree's own preorder gives the same tree.
    AirportBST r;
    int stoppedAt = 0;
    CHECK(r.buildFromPreorder(t.preorder(), stoppedAt) && stoppedAt == -1);
    CHECK(r.levelOrder() == t.levelOrder() && r.size() == 8 && r.validate().ok);

    // Lecture assignment A6: preorder 8 5 1 7 10 12 -> 8 (5: 1, 7), (10: right 12).
    AirportBST a6;
    CHECK(a6.buildFromPreorder(aps({"K08", "K05", "K01", "K07", "K10", "K12"}), stoppedAt));
    CHECK(a6.levelOrder() == (std::vector<std::string>{"K08", "K05", "K10", "K01", "K07", "K12"}));

    // Not the preorder of any BST: rejected, and the tree that was there is kept.
    std::vector<std::string> before = t.levelOrder();
    CHECK(!t.buildFromPreorder(aps({"K40", "K20", "K60", "K10"}), stoppedAt) && stoppedAt == 3);
    CHECK(!t.buildFromPreorder(aps({"K20", "K10", "K15", "K05"}), stoppedAt) && stoppedAt == 3);
    CHECK(!t.buildFromPreorder(aps({"K40", "K20", "K40"}), stoppedAt) && stoppedAt == 2);   // duplicate
    CHECK(t.levelOrder() == before && t.size() == 8 && t.validate().ok);

    // An empty list gives an empty tree.
    CHECK(t.buildFromPreorder({}, stoppedAt) && t.isEmpty() && t.height() == -1);
}

static void testFormatAndDetection(const std::string& savedPath) {
    Airport bos("General Edward Lawrence Logan International Airport", "Boston", "United States",
                "BOS", "KBOS", 42.36429977, -71.00520325);
    CHECK(formatSavedLine(bos) ==
          "\"BOS\",\"General Edward Lawrence Logan International Airport\",\"Boston\",\"United States\",\"KBOS\",42.36429977,-71.00520325");
    Airport quoted("Magdeburg \"City\" Airport", "Magdeburg", "Germany", "ZMG", "", 0.0, 1.0 / 3.0);
    std::string line = formatSavedLine(quoted);
    CHECK(line.find("\"Magdeburg \"\"City\"\" Airport\"") != std::string::npos);
    CHECK(line.find(",\"\",0,") != std::string::npos);               // empty ICAO, zero latitude
    std::vector<std::string> f = splitCsvLine(line);                // reads back exactly
    CHECK(f.size() == 7 && f[1] == "Magdeburg \"City\" Airport" && f[4].empty());
    CHECK(f.size() == 7 && std::stod(f[6]) == 1.0 / 3.0);

    CHECK(detectFileKind("data/airports.dat") == FileKind::AirportsDat);
    CHECK(detectFileKind(savedPath) == FileKind::SavedTree);
    writeLines("tests/bin/empty_tree.txt", {});
    CHECK(detectFileKind("tests/bin/empty_tree.txt") == FileKind::SavedTree);
    writeLines("tests/bin/five_fields.txt", {"a,b,c,d,e"});
    CHECK(detectFileKind("tests/bin/five_fields.txt") == FileKind::Unknown);
    CHECK(detectFileKind("tests/bin/no_such_file.txt") == FileKind::CannotOpen);
}

// SAVE then LOAD-TREE into a new tree; everything about the shape and the data must match.
static void checkRoundTrip(const char* label, const AirportBST& tree, const std::string& path) {
    std::vector<Airport> pre = tree.preorder();
    CHECK(saveAirports(path, pre));
    std::vector<std::string> lines = readLines(path);
    CHECK(static_cast<int>(lines.size()) == tree.size());

    std::vector<Airport> list;
    CHECK(readSavedAirports(path, list) == 0);
    CHECK(sameRecords(list, pre));                                  // the file holds the preorder

    AirportBST reloaded;
    int stoppedAt = 0;
    CHECK(reloaded.buildFromPreorder(list, stoppedAt));
    bool sameShape = reloaded.height() == tree.height() &&
                     reloaded.averageDepth() == tree.averageDepth() &&
                     reloaded.levelOrder() == tree.levelOrder();
    CHECK(sameShape);
    CHECK(sameRecords(reloaded.inorder(), tree.inorder()));
    ValidateResult v = reloaded.validate();
    CHECK(v.ok && v.reached == tree.size());

    std::string again = path + ".again";
    CHECK(saveAirports(again, reloaded.preorder()));
    bool identicalFile = readFile(again) == readFile(path);
    CHECK(identicalFile);

    std::cout << "  " << label << ": " << lines.size() << " lines; reloaded height " << reloaded.height()
              << " (was " << tree.height() << "), same level order: " << (sameShape ? "yes" : "no")
              << ", validate " << v.describe() << ", saved again byte-identical: "
              << (identicalFile ? "yes" : "no") << "\n";
}

static void testRoundTrips(const std::vector<Airport>& records) {
    std::cout << "save / load round trips:\n";
    AirportBST tree;
    tree.buildFromArray(records);
    checkRoundTrip("file order", tree, "tests/bin/rt_file_order.txt");

    std::vector<std::string> first = readLines("tests/bin/rt_file_order.txt");
    CHECK(!first.empty() && first[0].rfind("\"GKA\",", 0) == 0);   // the root comes first

    tree.remove("BOS");
    checkRoundTrip("after remove BOS", tree, "tests/bin/rt_no_bos.txt");

    tree.insert(Airport("Yuxuan Wei International Airport", "Boston", "United States", "QQQ", "", 0.0, 0.0));
    checkRoundTrip("with QQQ added", tree, "tests/bin/rt_qqq.txt");
    std::vector<Airport> list;
    readSavedAirports("tests/bin/rt_qqq.txt", list);
    AirportBST withQqq;
    int stoppedAt = 0;
    withQqq.buildFromPreorder(list, stoppedAt);
    FindResult q = withQqq.find("QQQ");
    CHECK(q.found && q.airport.getName() == "Yuxuan Wei International Airport" && q.airport.getIcao().empty());
    CHECK(!withQqq.find("BOS").found);

    std::vector<Airport> sorted = records;
    std::sort(sorted.begin(), sorted.end(),
              [](const Airport& a, const Airport& b) { return a.getIata() < b.getIata(); });
    AirportBST chain;
    chain.buildFromArray(sorted);
    checkRoundTrip("sorted chain (B3 recursion 6,072 deep)", chain, "tests/bin/rt_chain.txt");
}

static void testDamagedFiles() {
    std::cout << "damaged saved files (the tree loaded before must stay):\n";
    AirportBST tree;
    std::vector<Airport> list;
    readSavedAirports("tests/bin/rt_file_order.txt", list);
    int stoppedAt = 0;
    CHECK(tree.buildFromPreorder(list, stoppedAt));
    std::vector<std::string> before = tree.levelOrder();
    std::vector<std::string> good = readLines("tests/bin/rt_file_order.txt");
    int n = static_cast<int>(good.size());

    // The AAA line moved to the end: no longer a preorder; B3 stops at that last line.
    std::vector<std::string> moved = good;
    int aaa = -1;
    for (int i = 0; i < n; i++) {
        if (moved[i].rfind("\"AAA\",", 0) == 0) aaa = i;
    }
    CHECK(aaa >= 0);
    std::string aaaLine = moved[aaa];
    moved.erase(moved.begin() + aaa);
    moved.push_back(aaaLine);
    writeLines("tests/bin/bad_order.txt", moved);
    CHECK(readSavedAirports("tests/bin/bad_order.txt", list) == 0);
    CHECK(!tree.buildFromPreorder(list, stoppedAt) && stoppedAt == n - 1 && list[stoppedAt].getIata() == "AAA");
    std::cout << "  AAA line moved to the end: rejected at line " << stoppedAt + 1 << " ("
              << list[stoppedAt].getIata() << ")\n";

    // The root line repeated at the end: a duplicate.
    std::vector<std::string> dup = good;
    dup.push_back(good[0]);
    writeLines("tests/bin/bad_duplicate.txt", dup);
    CHECK(readSavedAirports("tests/bin/bad_duplicate.txt", list) == 0);
    CHECK(!tree.buildFromPreorder(list, stoppedAt) && stoppedAt == n);
    std::cout << "  root line repeated at the end: rejected at line " << stoppedAt + 1 << " ("
              << list[stoppedAt].getIata() << ")\n";

    // Lines that are not saved airports: reported by line number.
    std::vector<std::string> sixFields = good;
    sixFields[9] = sixFields[9].substr(0, sixFields[9].rfind(','));     // drop the longitude
    writeLines("tests/bin/bad_fields.txt", sixFields);
    CHECK(readSavedAirports("tests/bin/bad_fields.txt", list) == 10 && list.empty());
    std::vector<std::string> badNumber = good;
    badNumber[4] = "\"XXX\",\"Name\",\"City\",\"Country\",\"XXXX\",abc,0";
    writeLines("tests/bin/bad_number.txt", badNumber);
    CHECK(readSavedAirports("tests/bin/bad_number.txt", list) == 5);
    std::vector<std::string> longCode = good;
    longCode[2] = "\"XXXX\",\"Name\",\"City\",\"Country\",\"XXXX\",1,2";
    writeLines("tests/bin/bad_code.txt", longCode);
    CHECK(readSavedAirports("tests/bin/bad_code.txt", list) == 3);
    CHECK(readSavedAirports("tests/bin/no_such_file.txt", list) == -1);

    CHECK(tree.levelOrder() == before && tree.size() == n && tree.validate().ok);
}

int main() {
    testPreorderAndB3();

    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load) && records.size() == 6072);
    testRoundTrips(records);
    testFormatAndDetection("tests/bin/rt_file_order.txt");
    testDamagedFiles();
    return finishTests();
}
