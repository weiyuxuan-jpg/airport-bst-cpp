// Command-loop checks: a scripted session through runCommandLoop must print the lines of
// the project's sample session (load, find BOS, find XYZ) and handle the other commands
// and bad input. Run from the project root.

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CommandLoop.h"
#include "check.h"

static std::string run(const std::string& script, const std::vector<std::string>& startup = {}) {
    std::istringstream in(script);
    std::ostringstream out;
    runCommandLoop(in, out, true, startup);
    return out.str();
}

static bool has(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

static int countLines(const std::string& text) {
    int lines = 0;
    int n = static_cast<int>(text.size());
    for (int i = 0; i < n; i++) {
        if (text[i] == '\n') lines++;
    }
    return lines;
}

int main() {
    // Commands before any load: the tree is empty.
    std::string empty = run("find BOS\nmin\nstats\nlist\n");
    CHECK(has(empty, "> find BOS\nPath: (NULL)   not found (comparisons: 0)\n"));
    CHECK(has(empty, "> min\nThe tree is empty.\n"));
    CHECK(has(empty, "Airports: 0   Height: -1   Average depth: 0.0\n"));
    CHECK(has(empty, "0 airports listed in code order; sorted: yes\n"));

    // The sample session, lines 1-8 (the full path is printed where the sample shows "...").
    std::string s = run("load data/airports.dat\nfind BOS\nfind xyz\nmin\nmax\nstats\nquit\nfind BOS\n");
    CHECK(has(s, "> load data/airports.dat\n"
                 "Loaded 6072 airports (1626 lines skipped: no IATA code). Duplicates: 0\n"
                 "Height: 177   Average depth: 18.3\n"));
    CHECK(has(s, "> find BOS\nPath: GKA -> "));
    CHECK(has(s, " -> BOS   (comparisons: 12)\n"
                 "BOS  General Edward Lawrence Logan International Airport, Boston, United States"));
    CHECK(has(s, "> find xyz\nPath: GKA -> "));              // lower case is accepted
    CHECK(has(s, " -> (NULL)   not found (comparisons: 14)\n"));
    CHECK(has(s, "Minimum: AAA  "));
    CHECK(has(s, "Maximum: ZZV  "));
    CHECK(has(s, "Airports: 6072   Height: 177   Average depth: 18.3\n"));
    // Nothing runs after quit: "find BOS" appears only once.
    CHECK(s.find("> find BOS") == s.rfind("> find BOS"));

    // list prints every airport, then the sorted check.
    std::string l = run("list\n", {"load data/airports.dat"});
    CHECK(has(l, "6072 airports listed in code order; sorted: yes\n"));
    CHECK(has(l, "\nAAA  ") && has(l, "\nZZV  "));
    CHECK(countLines(l) == 1 + 2 + 1 + 6072 + 1 + 1);   // startup echo, load (2), list echo, airports, summary, final newline

    // Bad input.
    std::string bad = run("load data/airports.dat\nload no/such/file.dat\nstats\nfind\nload\nfly BOS\n# a comment\n\n");
    CHECK(has(bad, "Error: cannot open no/such/file.dat (the tree is unchanged)\n"));
    CHECK(has(bad, "> stats\nAirports: 6072   "));        // the old tree is still there
    CHECK(has(bad, "> find\nUsage: find CODE\n"));
    CHECK(has(bad, "> load\nUsage: load FILE\n"));
    CHECK(has(bad, "Unknown command 'fly'. Type help for the list.\n"));
    CHECK(!has(bad, "Unknown command '#"));

    // remove: the sample session's line 12 exactly, the root, not found.
    std::string rm = run("remove bos\nfind BOS\nremove BOS\nremove GKA\nstats\nremove\n",
                         {"load data/airports.dat"});
    CHECK(has(rm, "> remove bos\nDeleted BOS (case D2: only a left child, BOR)   validate: OK\n"));
    CHECK(has(rm, "> find BOS\nPath: GKA -> "));
    // BOR moved up into BOS's place (same depth), and no code lies between BOR and BOS.
    CHECK(has(rm, "-> BOU -> BOR -> (NULL)   not found (comparisons: 12)\n"));
    CHECK(has(rm, "> remove BOS\nBOS is not in the tree.\n"));
    CHECK(has(rm, "> remove GKA\nDeleted GKA (case D3: two children, successor "));
    CHECK(has(rm, "; GKA was the root, new root: GKE   validate: OK\n"));
    CHECK(has(rm, "> stats\nAirports: 6070   "));
    CHECK(has(rm, "> remove\nUsage: remove CODE\n"));

    // levels [k] [CODE]
    std::string lv = run("levels\nlevels 1\nlevels 2 aey\nlevels xyz\nlevels 9\nlevels 0\n",
                         {"load data/airports.dat"});
    CHECK(has(lv, "> levels\nLevel 0: GKA\nLevel 1: AEY MAG\nLevel 2: AAE EGS HGU POM\n> "));
    CHECK(has(lv, "> levels 1\nLevel 0: GKA\n> "));
    CHECK(has(lv, "> levels 2 aey\nLevels from AEY:\nLevel 1: AEY\nLevel 2: AAE EGS\n> "));
    CHECK(has(lv, "> levels xyz\nXYZ is not in the tree.\n"));
    CHECK(has(lv, "> levels 9\nUsage: levels [k] [CODE]"));
    CHECK(has(lv, "> levels 0\nUsage: levels [k] [CODE]"));
    CHECK(has(run("levels\n"), "> levels\nThe tree is empty.\n"));

    // add (Task 3(c)): the QQQ airport, then find and remove it.
    std::string q = run("find QQQ\n"
                        "add QQQ Yuxuan Wei International Airport, Boston, United States\n"
                        "find QQQ\nstats\nremove QQQ\nfind QQQ\nstats\n",
                        {"load data/airports.dat"});
    CHECK(has(q, "> add QQQ Yuxuan Wei International Airport, Boston, United States\n"
                 "Added QQQ  Yuxuan Wei International Airport, Boston, United States (depth 21)   validate: OK\n"));
    CHECK(has(q, " -> QQT -> QQQ   (comparisons: 22)\n"
                 "QQQ  Yuxuan Wei International Airport, Boston, United States  [ICAO -, lat 0.0000, lon 0.0000]\n"));
    CHECK(has(q, "> stats\nAirports: 6073   "));
    CHECK(has(q, "> remove QQQ\nDeleted QQQ (case D1: leaf)   validate: OK\n"));
    CHECK(has(q, " -> QQT -> (NULL)   not found (comparisons: 21)\n"));
    CHECK(has(q, "> stats\nAirports: 6072   Height: 177   Average depth: 18.3\n"));

    // add: all six fields, a quoted name with a comma, lower-case code, and bad input.
    std::string a = run("add qqr \"Comma, Field\", Testville, Testland, QQRR, 42.5, -71.25\n"
                        "find QQR\n"
                        "add\nadd QQ Name, City, Country\nadd Q-Q Name, City, Country\n"
                        "add QQS Name, City\nadd QQS Name, , Country\n"
                        "add QQS Name, City, Country, QQSS, 91, 0\nadd QQS Name, City, Country, QQSS, abc, 0\n"
                        "add BOS Another Boston, Boston, United States\nstats\n",
                        {"load data/airports.dat"});
    CHECK(has(a, "Added QQR  Comma, Field, Testville, Testland (depth "));
    CHECK(has(a, "QQR  Comma, Field, Testville, Testland  [ICAO QQRR, lat 42.5000, lon -71.2500]\n"));
    CHECK(has(a, "> add\nUsage: add CODE NAME, CITY, COUNTRY[, ICAO, LAT, LON]\n"));
    CHECK(has(a, "> add QQ Name, City, Country\nBad code 'QQ': a code has 3 letters or digits.\n"));
    CHECK(has(a, "> add Q-Q Name, City, Country\nBad code 'Q-Q'"));
    CHECK(has(a, "> add QQS Name, City\nUsage: add"));
    CHECK(has(a, "> add QQS Name, , Country\nUsage: add"));
    CHECK(has(a, "> add QQS Name, City, Country, QQSS, 91, 0\nBad coordinates '91, 0'"));
    CHECK(has(a, "> add QQS Name, City, Country, QQSS, abc, 0\nBad coordinates 'abc, 0'"));
    CHECK(has(a, "> add BOS Another Boston, Boston, United States\nBOS is already in the tree (rule I4); nothing was added.\n"));
    CHECK(has(a, "> stats\nAirports: 6073   "));          // only QQR was added

    // save / load (Task 4(b)): save, change the tree, load the saved file back.
    std::string sl = run("save tests/bin/cmd_tree.txt\nremove BOS\nstats\n"
                         "load tests/bin/cmd_tree.txt\nfind BOS\nlevels\nsave\n",
                         {"load data/airports.dat"});
    CHECK(has(sl, "> save tests/bin/cmd_tree.txt\nSaved 6072 airports in preorder to tests/bin/cmd_tree.txt\n"));
    CHECK(has(sl, "> stats\nAirports: 6071   "));
    CHECK(has(sl, "> load tests/bin/cmd_tree.txt\n"
                  "Rebuilt the tree from 6072 airports in preorder (tests/bin/cmd_tree.txt)\n"
                  "Height: 177   Average depth: 18.3\nvalidate: OK\n"));
    CHECK(has(sl, " -> BOS   (comparisons: 12)\n"));          // BOS is back, in the same place
    CHECK(has(sl, "> levels\nLevel 0: GKA\nLevel 1: AEY MAG\nLevel 2: AAE EGS HGU POM\n"));
    CHECK(has(sl, "> save\nUsage: save FILE\n"));

    // load of bad files keeps the tree.
    {
        std::ofstream notPreorder("tests/bin/cmd_not_preorder.txt");
        notPreorder << "\"K40\",\"A\",\"B\",\"C\",\"\",0,0\n\"K20\",\"A\",\"B\",\"C\",\"\",0,0\n"
                       "\"K60\",\"A\",\"B\",\"C\",\"\",0,0\n\"K10\",\"A\",\"B\",\"C\",\"\",0,0\n";
        std::ofstream unknown("tests/bin/cmd_unknown.txt");
        unknown << "a,b,c\n";
        std::ofstream badLine("tests/bin/cmd_bad_line.txt");
        badLine << "\"K40\",\"A\",\"B\",\"C\",\"\",0,0\n\"K20\",\"A\",\"B\"\n";
    }
    std::string bf = run("load tests/bin/cmd_not_preorder.txt\nload tests/bin/cmd_unknown.txt\n"
                         "load tests/bin/cmd_bad_line.txt\nload tests/bin/no_such_tree.txt\nstats\n",
                         {"load data/airports.dat"});
    CHECK(has(bf, "Error: tests/bin/cmd_not_preorder.txt is not the preorder of a BST: line 4 (K10) does not fit; the tree is unchanged\n"));
    CHECK(has(bf, "Error: tests/bin/cmd_unknown.txt is neither airports.dat (14 fields per line) nor a saved tree (7 fields per line); the tree is unchanged\n"));
    CHECK(has(bf, "Error: line 2 of tests/bin/cmd_bad_line.txt is not a saved airport"));
    CHECK(has(bf, "Error: cannot open tests/bin/no_such_tree.txt (the tree is unchanged)\n"));
    CHECK(has(bf, "> stats\nAirports: 6072   Height: 177   Average depth: 18.3\n"));

    // range (Task 5): the sample session's line 10, then the airports in code order.
    std::string rg = run("range BOS BWI\nrange bwi bos\nrange XYZ XYZ\nrange GKA GKA\nrange BOS\n",
                         {"load data/airports.dat"});
    CHECK(has(rg, "> range BOS BWI\n113 airports, 127 nodes visited\n"
                  "BOS  General Edward Lawrence Logan International Airport, Boston, United States\n"
                  "BOU  Bourges Airport, Bourges, France\n"));
    CHECK(has(rg, "BWI  Baltimore/Washington International Thurgood Marshall Airport, Baltimore, United States\n> range bwi bos\n"));
    CHECK(has(rg, "> range bwi bos\nFROM must not come after TO (BWI > BOS).\n"));
    CHECK(has(rg, "> range XYZ XYZ\n0 airports, 14 nodes visited\n"));     // same path as find XYZ
    CHECK(has(rg, "> range GKA GKA\n1 airports, "));
    CHECK(has(rg, "> range BOS\nUsage: range FROM TO\n"));
    CHECK(has(run("range AAA ZZZ\n"), "> range AAA ZZZ\n0 airports, 0 nodes visited\n"));   // empty tree

    // rebalance: sample session lines 11-14 exactly.
    std::string rb = run("remove BOS\nrebalance\nvalidate\n", {"load data/airports.dat"});
    CHECK(has(rb, "> remove BOS\nDeleted BOS (case D2: only a left child, BOR)   validate: OK\n"
                  "> rebalance\nHeight: 12   Average depth: 10.7\n"));
    CHECK(has(rb, "validate: OK (6071 nodes"));

    // experiment: the three rows; a missing file; the current tree is not touched.
    std::string ex = run("experiment\nexperiment no/such/file.dat\nstats\n", {"load data/airports.dat"});
    CHECK(has(ex, "Shape experiment on 6072 airports from data/airports.dat (the current tree is not changed)\n"));
    CHECK(has(ex, "10000 random searches (seed 2560), time = median of 7 runs\n"));
    CHECK(has(ex, "File order                  177       18.3"));
    CHECK(has(ex, "Sorted order               6071     3035.5"));
    CHECK(has(ex, "Balanced (middle rule)       12       10.7"));
    CHECK(has(ex, "Random shuffle               26       14.2"));
    CHECK(has(ex, "(Random shuffle uses seed 2560.)\n"
                  "Random shuffle over 100 seeds (1 to 100): height min 24, median 28, max 34; mean average depth 14.57\n"));
    CHECK(!has(ex, "FAILED"));
    CHECK(has(ex, "> experiment no/such/file.dat\nError: cannot open no/such/file.dat\n"));
    CHECK(has(ex, "> stats\nAirports: 6072   Height: 177   Average depth: 18.3\n"));

    // validate on its own: empty tree, loaded tree, after deletes.
    std::string v = run("validate\nload data/airports.dat\nvalidate\nremove AEY\nremove GKA\nvalidate\n");
    CHECK(has(v, "> validate\nvalidate: OK (0 nodes, each inside its (lo, hi) range)\n"));
    CHECK(has(v, "> validate\nvalidate: OK (6072 nodes, each inside its (lo, hi) range)\n"));
    CHECK(has(v, "> validate\nvalidate: OK (6070 nodes, each inside its (lo, hi) range)\n"));
    CHECK(!has(v, "FAILED"));

    std::string help = run("help\n");
    CHECK(has(help, "find CODE") && has(help, "add CODE") && has(help, "remove CODE") &&
          has(help, "validate") && has(help, "save FILE") && has(help, "range FROM TO") &&
          has(help, "rebalance") && has(help, "experiment") && has(help, "list") && has(help, "quit"));

    return finishTests();
}
