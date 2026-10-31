#include "AirportLoader.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace {

// Column positions in airports.dat (0-based); fields 9-14 are ignored.
const int kFieldCount = 14;
const int kName = 1;
const int kCity = 2;
const int kCountry = 3;
const int kIata = 4;
const int kIcao = 5;
const int kLatitude = 6;
const int kLongitude = 7;

const std::string kMissing = "\\N";

// A saved tree has 7 fields: code, name, city, country, ICAO, latitude, longitude.
const int kSavedFieldCount = 7;

// QUOTE: the text in double quotes, each " written as "".
std::string quote(const std::string& text) {
    std::string out = "\"";
    int n = static_cast<int>(text.size());
    for (int i = 0; i < n; i++) {
        if (text[i] == '"') out += "\"\"";
        else out += text[i];
    }
    return out + "\"";
}

// NUM: the fewest significant digits (15, 16 or 17) that read back as exactly value.
// 17 digits always do, so the loop never runs out.
std::string formatNumber(double value) {
    char buffer[40];
    for (int i = 0; i < 3; i++) {
        int digits = 15 + i;
        std::snprintf(buffer, sizeof(buffer), "%.*g", digits, value);
        if (std::strtod(buffer, nullptr) == value) break;
    }
    return buffer;
}

// True if the whole of text is a finite number (strtod alone would accept "12abc").
bool parseNumber(const std::string& text, double& value) {
    if (text.empty()) return false;
    char* end = nullptr;
    errno = 0;
    value = std::strtod(text.c_str(), &end);
    return end == text.c_str() + text.size() && errno != ERANGE && std::isfinite(value);
}

}  // namespace

bool parseCoordinates(const std::string& latitudeText, const std::string& longitudeText,
                      double& latitude, double& longitude) {
    return parseNumber(latitudeText, latitude) && parseNumber(longitudeText, longitude) &&
           std::fabs(latitude) <= 90.0 && std::fabs(longitude) <= 180.0;
}

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;                          // step 1
    std::string cur;
    bool inQuotes = false;
    int n = static_cast<int>(line.size());
    for (int i = 0; i < n; i++) {                             // step 2
        char c = line[i];                                     // step 3
        if (inQuotes) {                                       // step 4
            if (c == '"' && i + 1 < n && line[i + 1] == '"') {
                cur += '"';                                   // step 6: "" -> "
                i++;
            } else if (c == '"') {
                inQuotes = false;                             // step 7: closing quote
            } else {
                cur += c;                                     // step 8: a comma here is text
            }
        } else if (c == '"') {
            inQuotes = true;                                  // step 10: opening quote
        } else if (c == ',') {
            fields.push_back(cur);                            // step 11: field ends
            cur.clear();
        } else {
            cur += c;                                         // step 12
        }
    }
    fields.push_back(cur);                                    // step 13: the last field
    return fields;
}

bool loadAirports(const std::string& path, std::vector<Airport>& records, LoadReport& report) {
    records.clear();
    report = LoadReport();

    std::ifstream in(path);                                   // step 1
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {                          // step 3
        report.lines++;                                       // step 4
        if (!line.empty() && line.back() == '\r') line.pop_back();   // step 5

        std::vector<std::string> f = splitCsvLine(line);      // step 6
        if (static_cast<int>(f.size()) != kFieldCount) {      // step 7
            report.malformed++;
            report.malformedLines.push_back(report.lines);
            continue;
        }
        if (f[kIata] == kMissing) {                           // step 8: no IATA code
            report.skipped++;
            continue;
        }

        double latitude = 0.0;
        double longitude = 0.0;
        bool coordinatesOk = parseCoordinates(f[kLatitude], f[kLongitude], latitude, longitude);
        if (f[kIata].size() != 3 || !coordinatesOk) {         // step 9
            report.malformed++;
            report.malformedLines.push_back(report.lines);
            continue;
        }

        std::string icao = (f[kIcao] == kMissing) ? "" : f[kIcao];   // step 11
        records.push_back(Airport(f[kName], f[kCity], f[kCountry],   // step 12
                                  f[kIata], icao, latitude, longitude));
    }
    return true;
}

FileKind detectFileKind(const std::string& path) {
    std::ifstream in(path);
    if (!in) return FileKind::CannotOpen;                     // step 1
    std::string line;
    if (!std::getline(in, line)) return FileKind::SavedTree;  // step 2: empty file
    if (!line.empty() && line.back() == '\r') line.pop_back();
    int n = static_cast<int>(splitCsvLine(line).size());      // step 3
    if (n == kFieldCount) return FileKind::AirportsDat;       // step 4
    if (n == kSavedFieldCount) return FileKind::SavedTree;    // step 5
    return FileKind::Unknown;                                 // step 6
}

std::string formatSavedLine(const Airport& airport) {
    return quote(airport.getIata()) + "," + quote(airport.getName()) + "," +
           quote(airport.getCity()) + "," + quote(airport.getCountry()) + "," +
           quote(airport.getIcao()) + "," + formatNumber(airport.getLatitude()) + "," +
           formatNumber(airport.getLongitude());
}

bool saveAirports(const std::string& path, const std::vector<Airport>& records) {
    std::ofstream out(path);                                  // SAVE step 1
    if (!out) return false;
    int n = static_cast<int>(records.size());
    for (int i = 0; i < n; i++) out << formatSavedLine(records[i]) << "\n";   // steps 3-5
    out.flush();
    return static_cast<bool>(out);
}

int readSavedAirports(const std::string& path, std::vector<Airport>& records) {
    records.clear();
    std::ifstream in(path);                                   // LOAD-TREE step 1
    if (!in) return -1;
    std::string line;
    int lineNumber = 0;
    while (std::getline(in, line)) {                          // step 3
        lineNumber++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<std::string> f = splitCsvLine(line);      // step 4
        double latitude = 0.0;
        double longitude = 0.0;
        if (static_cast<int>(f.size()) != kSavedFieldCount || f[0].size() != 3 ||
            !parseCoordinates(f[5], f[6], latitude, longitude)) {
            records.clear();                                  // step 5: bad line
            return lineNumber;
        }
        records.push_back(Airport(f[1], f[2], f[3], f[0], f[4], latitude, longitude));   // step 6
    }
    return 0;
}

int countDuplicateCodes(const std::vector<Airport>& records) {
    int n = static_cast<int>(records.size());
    std::vector<std::string> codes;
    for (int i = 0; i < n; i++) codes.push_back(records[i].getIata());
    std::sort(codes.begin(), codes.end());
    int duplicates = 0;
    for (int i = 0; i < n - 1; i++) {
        if (codes[i] == codes[i + 1]) duplicates++;           // equal codes are now neighbours
    }
    return duplicates;
}
