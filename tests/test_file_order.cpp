// Task 14: the evidence for docs/shape_experiment.md, "Why the file order gives height 177".
// Each claim of the explanation is measured here and checked.
//   1. how the file is ordered: by airport ID; lines 21-205 are Canada in ICAO order, and
//      every Canadian IATA code there is the ICAO code without the leading "C"
//   2. ascending runs of IATA codes in file order, against a random shuffle
//   3. the deepest path of the file-order tree: where its nodes come from in the file and
//      which way each step goes
//   4. how much of the average depth the long run causes
//   5. random shuffles over 100 seeds against the theory for random insertion
// Run from the project root.

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "ShapeExperiment.h"
#include "check.h"

// One line of airports.dat with an IATA code, with what the loader does not keep.
struct FileEntry {
    int line = 0;
    int id = 0;
    std::string iata;
    std::string icao;
    std::string country;
};

struct Run {
    int start = 0;     // index into the code list
    int length = 0;
};

// Maximal stretches in which every code is larger than the one before.
static std::vector<Run> ascendingRuns(const std::vector<std::string>& codes) {
    std::vector<Run> runs;
    int n = static_cast<int>(codes.size());
    Run cur;
    for (int i = 0; i < n; i++) {
        if (i > 0 && codes[i] <= codes[i - 1]) {
            runs.push_back(cur);
            cur = Run();
            cur.start = i;
        }
        cur.length++;
    }
    if (n > 0) runs.push_back(cur);
    return runs;
}

static Run longestRun(const std::vector<Run>& runs) {
    Run best;
    int n = static_cast<int>(runs.size());
    for (int i = 0; i < n; i++) {
        if (runs[i].length > best.length) best = runs[i];
    }
    return best;
}

static double increasingShare(const std::vector<std::string>& codes) {
    int n = static_cast<int>(codes.size());
    int up = 0;
    for (int i = 0; i < n - 1; i++) {
        if (codes[i] < codes[i + 1]) up++;
    }
    return n > 1 ? static_cast<double>(up) / (n - 1) : 0.0;
}

int main() {
    // ---- 1. How the lines are ordered.
    std::ifstream in("data/airports.dat");
    std::string text;
    std::vector<FileEntry> entries;
    std::vector<int> allIds;
    int lineNumber = 0;
    while (std::getline(in, text)) {
        lineNumber++;
        std::vector<std::string> f = splitCsvLine(text);
        if (f.size() != 14) continue;
        allIds.push_back(std::stoi(f[0]));
        if (f[4] == "\\N") continue;
        FileEntry e;
        e.line = lineNumber;
        e.id = allIds.back();
        e.iata = f[4];
        e.icao = f[5];
        e.country = f[3];
        entries.push_back(e);
    }
    CHECK(allIds.size() == 7698 && entries.size() == 6072);
    bool idsIncrease = true;
    int idCount = static_cast<int>(allIds.size());
    for (int i = 0; i < idCount - 1; i++) {
        if (allIds[i] >= allIds[i + 1]) idsIncrease = false;
    }
    CHECK(idsIncrease);

    // The same codes in the same order as the loader gives.
    std::vector<Airport> records;
    LoadReport load;
    CHECK(loadAirports("data/airports.dat", records, load));
    int n = static_cast<int>(entries.size());
    bool sameOrder = static_cast<int>(records.size()) == n;
    int compareCount = sameOrder ? n : 0;                   // never index past records
    for (int i = 0; i < compareCount; i++) {
        if (records[i].getIata() != entries[i].iata) sameOrder = false;
    }
    CHECK(sameOrder);
    std::map<std::string, FileEntry> byCode;
    for (int i = 0; i < n; i++) byCode[entries[i].iata] = entries[i];

    // Lines 21-205: one Canada block; ICAO in order (one entry out of place); IATA = ICAO minus "C".
    int canadaCodes = 0;
    int icaoDescents = 0;
    int descentLine = 0;
    bool allCanada = true;
    bool iataIsIcaoMinusC = true;
    std::string previousIcao;
    for (int i = 0; i < n; i++) {
        const FileEntry& e = entries[i];
        if (e.line < 21 || e.line > 205) continue;
        canadaCodes++;
        if (e.country != "Canada") allCanada = false;
        if (e.icao != "C" + e.iata) iataIsIcaoMinusC = false;
        if (!previousIcao.empty() && e.icao < previousIcao) {
            icaoDescents++;
            descentLine = e.line;
        }
        previousIcao = e.icao;
    }
    CHECK(allCanada && canadaCodes == 181 && iataIsIcaoMinusC);
    CHECK(icaoDescents == 1 && descentLine == 42);
    CHECK(byCode["YZS"].line == 41);                         // CYZS is the one out of place
    std::cout << "1. IDs increase along all " << idCount << " lines. Lines 21-205: " << canadaCodes
              << " Canadian airports with IATA codes, IATA = ICAO without the leading C for all of them,\n"
              << "   ICAO in increasing order except once (CYZS on line 41 comes before CYDA on line 42).\n";

    // ---- 2. Ascending runs in file order and after a shuffle.
    std::vector<std::string> fileCodes;
    for (int i = 0; i < n; i++) fileCodes.push_back(entries[i].iata);
    std::vector<Run> runs = ascendingRuns(fileCodes);
    Run longest = longestRun(runs);
    CHECK(longest.length == 162);
    CHECK(entries[longest.start].line == 42 && entries[longest.start + longest.length - 1].line == 205);
    CHECK(fileCodes[longest.start] == "YDA" && fileCodes[longest.start + longest.length - 1] == "ZFM");
    int runsOf20 = 0;
    int runCount = static_cast<int>(runs.size());
    std::vector<int> lengths;
    for (int i = 0; i < runCount; i++) {
        if (runs[i].length >= 20) runsOf20++;
        lengths.push_back(runs[i].length);
    }
    std::sort(lengths.rbegin(), lengths.rend());
    CHECK(lengths.size() >= 7 && lengths[1] == 35 && lengths[2] == 30 && lengths[3] == 29 &&
          lengths[4] == 26 && lengths[5] == 21 && lengths[6] < 20);
    Run second;                                              // the longest run apart from the Canadian one
    for (int i = 0; i < runCount; i++) {
        if (runs[i].length == 35) second = runs[i];
    }
    CHECK(entries[second.start].line == 4437 && entries[second.start + 34].line == 4471 &&
          entries[second.start].country == "United States");

    std::vector<Airport> shuffled = shuffledCopy(records, 2560);
    std::vector<std::string> shuffledCodes;
    for (int i = 0; i < n; i++) shuffledCodes.push_back(shuffled[i].getIata());
    std::vector<Run> shuffledRuns = ascendingRuns(shuffledCodes);
    Run shuffledLongest = longestRun(shuffledRuns);
    double fileUp = increasingShare(fileCodes);
    double shuffledUp = increasingShare(shuffledCodes);
    CHECK(fileUp > 0.55 && std::fabs(shuffledUp - 0.5) < 0.02);
    CHECK(shuffledLongest.length < 15);
    std::cout << "2. File order: " << fileUp * 100 << "% of neighbours increase, " << runCount
              << " ascending runs, " << runsOf20 << " of length >= 20, longest " << longest.length
              << " (lines 42-205, YDA ... ZFM);\n   the next longest: " << lengths[1] << ", " << lengths[2]
              << ", " << lengths[3] << ", " << lengths[4] << ", " << lengths[5]
              << " (the 35 are United States, lines 4437-4471).\n"
              << "   Shuffled (seed 2560): " << shuffledUp * 100 << "% increase, " << shuffledRuns.size()
              << " runs, longest " << shuffledLongest.length << ".\n";

    // ---- 3. The deepest path.
    AirportBST tree;
    tree.buildFromArray(records);
    std::vector<NodeInfo> nodes = tree.nodes();
    const NodeInfo& deepest = nodes.back();                   // breadth-first: the last is deepest
    CHECK(deepest.depth == 177 && deepest.code == "YZR");
    std::vector<std::string> path = tree.find("YZR").path;
    CHECK(path.size() == 178);
    int fromPng = 0, fromFirstCanadaRun = 0, fromLongRun = 0;
    for (int i = 0; i < 178; i++) {
        int line = byCode[path[i]].line;
        if (byCode[path[i]].country == "Papua New Guinea") fromPng++;
        else if (line >= 21 && line <= 41) fromFirstCanadaRun++;
        else if (line >= 42 && line <= 205) fromLongRun++;
    }
    CHECK(fromPng == 4 && fromFirstCanadaRun == 19 && fromLongRun == 155);
    int rightSteps = 0, leftSteps = 0;
    int firstLongRunIndex = 178 - fromLongRun;               // index of YDA on the path
    int stepsInRun = 177 - firstLongRunIndex;
    for (int k = 0; k < stepsInRun; k++) {
        int i = firstLongRunIndex + k;
        if (path[i + 1] > path[i]) rightSteps++;
        else leftSteps++;
    }
    CHECK(path[firstLongRunIndex] == "YDA" && path[firstLongRunIndex - 1] == "YZS");
    CHECK(rightSteps == 154 && leftSteps == 0);
    std::cout << "3. Deepest node YZR, depth 177. Its 178-node path: " << fromPng
              << " Papua New Guinea codes (" << path[0] << " " << path[1] << " " << path[2] << " " << path[3]
              << "), " << fromFirstCanadaRun << " Canadian codes from lines 21-41 (YAM ... YCY, YZS),\n"
              << "   then YZS -> left -> YDA and " << rightSteps << " steps, all to the right, through the long run"
              << " (" << fromLongRun << " codes, YDA ... YZR).\n";

    // ---- 4. What the long run costs in average depth.
    std::map<std::string, int> depthOf;
    int m = static_cast<int>(nodes.size());
    for (int i = 0; i < m; i++) depthOf[nodes[i].code] = nodes[i].depth;
    long long runDepth = 0;
    long long allDepth = 0;
    for (int i = 0; i < m; i++) allDepth += nodes[i].depth;
    for (int i = 0; i < longest.length; i++) runDepth += depthOf[fileCodes[longest.start + i]];
    double runAverage = static_cast<double>(runDepth) / longest.length;
    double share = static_cast<double>(runDepth) / n;        // part of the average depth
    double others = static_cast<double>(allDepth - runDepth) / (n - longest.length);
    CHECK(share > 2.0 && others < 18.3);
    std::cout << "4. The " << longest.length << " codes of the long run have depths averaging " << runAverage
              << "; they add " << share << " to the average depth " << tree.averageDepth()
              << ". The other " << n - longest.length << " nodes average " << others << ".\n";

    // ---- 5. Random shuffles against the theory.
    SeedSweep sweep = sweepSeeds(records, 1, 100);
    double harmonic = 0.0;
    for (int i = 0; i < n; i++) harmonic += 1.0 / (i + 1);
    double expectedAverage = 2.0 * (1.0 + 1.0 / n) * harmonic - 4.0;
    CHECK(sweep.minHeight >= 20 && sweep.maxHeight <= 40);
    CHECK(std::fabs(sweep.meanAverageDepth - expectedAverage) < 0.01 * expectedAverage);
    std::cout << "5. 100 shuffles (seeds 1-100): height " << sweep.minHeight << " to " << sweep.maxHeight
              << ", median " << sweep.medianHeight << "; mean average depth " << sweep.meanAverageDepth
              << " (theory for random order: 2(1 + 1/n)H_n - 4 = " << expectedAverage << ").\n";
    return finishTests();
}
