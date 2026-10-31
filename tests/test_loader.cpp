// Task 4 checks for the airports.dat loader (docs/pseudocode.md, section 1):
//   1. splitCsvLine on hand-written lines (quotes, "", \N, empty fields)
//   2. loadAirports on a small fixture with one line of every kind, in LF and CRLF form
//   3. loadAirports on the real data/airports.dat against the project's "facts" box
// Run from the project root: the paths below are relative to it.

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Airport.h"
#include "AirportLoader.h"
#include "check.h"

using Fields = std::vector<std::string>;

static void testSplit() {
    CHECK(splitCsvLine("a,b,c") == (Fields{"a", "b", "c"}));
    CHECK(splitCsvLine("1,\"Harstad/Narvik Airport, Evenes\",x") ==
          (Fields{"1", "Harstad/Narvik Airport, Evenes", "x"}));
    CHECK(splitCsvLine("\"Magdeburg \"\"City\"\" Airport\",y") ==
          (Fields{"Magdeburg \"City\" Airport", "y"}));
    CHECK(splitCsvLine("1,\\N,2") == (Fields{"1", "\\N", "2"}));     // \N stays as text
    CHECK(splitCsvLine(",,") == (Fields{"", "", ""}));
    CHECK(splitCsvLine("") == (Fields{""}));                         // one empty field
    CHECK(splitCsvLine("\"\",x") == (Fields{"", "x"}));              // quoted empty field
    CHECK(splitCsvLine("a,") == (Fields{"a", ""}));                  // trailing comma
    CHECK(splitCsvLine("\"\"\"\"") == (Fields{"\""}));               // a field that is one "
    CHECK(splitCsvLine("\"abc,def") == (Fields{"abc,def"}));         // unclosed quote: rest is text
}

static const char* kFixtureLines[] = {
    // 1 valid
    "1,\"Goroka Airport\",\"Goroka\",\"Papua New Guinea\",\"GKA\",\"AYGA\",-6.081689834590001,145.391998291,5282,10,\"U\",\"Pacific/Port_Moresby\",\"airport\",\"OurAirports\"",
    // 2 skipped: IATA is \N
    "5,\"Jacksons Test\",\"Port Moresby\",\"Papua New Guinea\",\\N,\"AYPY\",-9.44,147.22,146,10,\"U\",\"Pacific/Port_Moresby\",\"airport\",\"OurAirports\"",
    // 3 valid: comma and "" inside quotes
    "641,\"Harstad/Narvik Airport, \"\"Evenes\"\"\",\"Harstad/Narvik\",\"Norway\",\"EVE\",\"ENEV\",68.49,16.67,84,1,\"E\",\"Europe/Oslo\",\"airport\",\"OurAirports\"",
    // 4 malformed: 5 fields
    "9,\"Short line\",\"X\",\"Y\",\"ABC\"",
    // 5 malformed: latitude is not a number
    "10,\"Bad Lat\",\"X\",\"Y\",\"BLT\",\"XXXX\",abc,10.0,0,0,\"U\",\"Z\",\"airport\",\"OurAirports\"",
    // 6 malformed: IATA has 2 characters
    "11,\"Two Letter\",\"X\",\"Y\",\"AB\",\"XXXX\",1.0,2.0,0,0,\"U\",\"Z\",\"airport\",\"OurAirports\"",
    // 7 valid: ICAO is \N, stored as ""
    "12,\"No Icao Field\",\"Town\",\"Land\",\"NIC\",\\N,1.5,-2.5,0,0,\"U\",\"Z\",\"airport\",\"OurAirports\"",
    // 8 malformed: latitude out of range
    "13,\"Too North\",\"X\",\"Y\",\"TNO\",\"XXXX\",91.0,0.0,0,0,\"U\",\"Z\",\"airport\",\"OurAirports\"",
    // 9 valid but a duplicate code (GKA again)
    "14,\"Second Goroka\",\"Goroka\",\"Papua New Guinea\",\"GKA\",\"AYGB\",-6.0,145.0,0,0,\"U\",\"Z\",\"airport\",\"OurAirports\"",
    // 10 malformed: empty line
    "",
    // 11 valid, written without a final newline
    "15,\"Last Line\",\"End\",\"Land\",\"LST\",\"LSTX\",0.0,0.0,0,0,\"U\",\"Z\",\"airport\",\"OurAirports\"",
};

static void writeFixture(const std::string& path, const std::string& eol) {
    std::ofstream out(path, std::ios::binary);
    int n = static_cast<int>(sizeof(kFixtureLines) / sizeof(kFixtureLines[0]));
    for (int i = 0; i < n; i++) {
        out << kFixtureLines[i];
        if (i < n - 1) out << eol;
    }
}

static void checkFixture(const std::string& path) {
    std::vector<Airport> records;
    LoadReport report;
    CHECK(loadAirports(path, records, report));
    CHECK(report.lines == 11);
    CHECK(records.size() == 5);
    CHECK(report.skipped == 1);
    CHECK(report.malformed == 5);
    CHECK(report.malformedLines == (std::vector<int>{4, 5, 6, 8, 10}));
    CHECK(countDuplicateCodes(records) == 1);
    if (records.size() != 5) return;

    CHECK(records[0].getIata() == "GKA");
    CHECK(records[0].getLatitude() == -6.081689834590001);
    CHECK(records[1].getIata() == "EVE");
    CHECK(records[1].getName() == "Harstad/Narvik Airport, \"Evenes\"");
    CHECK(records[1].getCity() == "Harstad/Narvik");
    CHECK(records[2].getIata() == "NIC");
    CHECK(records[2].getIcao().empty());
    CHECK(records[2].getLongitude() == -2.5);
    CHECK(records[3].getName() == "Second Goroka");
    CHECK(records[4].getIata() == "LST");
    CHECK(records[4].getCountry() == "Land");   // no '\r' left on any field
}

static int findCode(const std::vector<Airport>& records, const std::string& code) {
    int n = static_cast<int>(records.size());
    for (int i = 0; i < n; i++) {
        if (records[i].getIata() == code) return i;
    }
    return -1;
}

static void testRealFile() {
    std::vector<Airport> records;
    LoadReport report;
    CHECK(loadAirports("data/airports.dat", records, report));

    // The "facts about the posted file" box of the project description.
    CHECK(report.lines == 7698);
    CHECK(records.size() == 6072);
    CHECK(report.skipped == 1626);
    CHECK(report.malformed == 0);
    CHECK(countDuplicateCodes(records) == 0);

    int n = static_cast<int>(records.size());
    std::vector<std::string> countries;
    int unitedStates = 0;
    bool allHaveIcao = true;
    for (int i = 0; i < n; i++) {
        countries.push_back(records[i].getCountry());
        if (records[i].getCountry() == "United States") unitedStates++;
        if (records[i].getIcao().empty()) allHaveIcao = false;
    }
    std::sort(countries.begin(), countries.end());
    int distinctCountries = static_cast<int>(
        std::unique(countries.begin(), countries.end()) - countries.begin());
    CHECK(distinctCountries == 235);
    CHECK(unitedStates == 1251);
    CHECK(allHaveIcao);   // the data has no valid-IATA record with ICAO \N

    CHECK(n > 0 && records[0].getIata() == "GKA");       // line 1
    CHECK(n > 0 && records[n - 1].getIata() == "CPO");   // line 7697 (7698 has no IATA)

    int bos = findCode(records, "BOS");
    CHECK(bos >= 0);
    if (bos >= 0) {
        const Airport& a = records[bos];
        CHECK(a.getName() == "General Edward Lawrence Logan International Airport");
        CHECK(a.getCity() == "Boston");
        CHECK(a.getCountry() == "United States");
        CHECK(a.getIcao() == "KBOS");
        CHECK(a.getLatitude() == 42.36429977);
        CHECK(a.getLongitude() == -71.00520325);
    }
    int zmg = findCode(records, "ZMG");
    CHECK(zmg >= 0 && records[zmg].getName() == "Magdeburg \"City\" Airport");
    int eve = findCode(records, "EVE");
    CHECK(eve >= 0 && records[eve].getName() == "Harstad/Narvik Airport, Evenes");
    int du9 = findCode(records, "DU9");   // kept: only \N is skipped (decision 2026-10-21)
    CHECK(du9 >= 0 && records[du9].getCity() == "Dunnville");

    std::cout << "real file: " << report.lines << " lines, " << records.size() << " airports, "
              << report.skipped << " skipped (no IATA code), " << report.malformed
              << " malformed, " << countDuplicateCodes(records) << " duplicates, "
              << distinctCountries << " countries, " << unitedStates << " in the United States\n";
}

int main() {
    testSplit();

    writeFixture("tests/bin/fixture_lf.dat", "\n");
    checkFixture("tests/bin/fixture_lf.dat");
    writeFixture("tests/bin/fixture_crlf.dat", "\r\n");   // same lines, Windows line ends
    checkFixture("tests/bin/fixture_crlf.dat");

    std::vector<Airport> none;
    LoadReport report;
    CHECK(!loadAirports("tests/bin/no_such_file.dat", none, report));
    CHECK(none.empty());

    testRealFile();
    return finishTests();
}
