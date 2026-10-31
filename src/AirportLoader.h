#ifndef AIRPORT_LOADER_H
#define AIRPORT_LOADER_H

#include <string>
#include <vector>

#include "Airport.h"

// Reading airports.dat (OpenFlights format). See docs/pseudocode.md, section 1.

// What LOAD-AIRPORTS found in the file.
struct LoadReport {
    int lines = 0;                    // lines read
    int skipped = 0;                  // IATA code is \N (no code)
    int malformed = 0;                // wrong field count, bad IATA length or bad coordinates
    std::vector<int> malformedLines;  // 1-based line numbers of the malformed lines
};

// SPLIT-CSV-LINE: splits one line at commas outside double quotes.
// Inside quotes a comma is text and "" stands for one " character; the quotes are removed.
std::vector<std::string> splitCsvLine(const std::string& line);

// LOAD-AIRPORTS: reads every line of path into records (replacing their old contents).
// Skips only lines whose IATA field is \N; returns false if the file cannot be opened.
bool loadAirports(const std::string& path, std::vector<Airport>& records, LoadReport& report);

// True if both texts are whole finite numbers with |latitude| <= 90 and |longitude| <= 180.
// Used by LOAD-AIRPORTS (step 9) and by the add command.
bool parseCoordinates(const std::string& latitudeText, const std::string& longitudeText,
                      double& latitude, double& longitude);

// Number of records whose IATA code already appeared earlier in the list
// (a code that appears k times counts k - 1). Independent of the tree.
int countDuplicateCodes(const std::vector<Airport>& records);

// ---- Saved trees (docs/pseudocode.md, section 8). One airport per line, 7 fields:
//   "BOS","General Edward Lawrence Logan International Airport","Boston","United States","KBOS",42.36429977,-71.00520325

// DETECT-FILE: which kind of file load was given (by the field count of its first line).
enum class FileKind {
    CannotOpen,
    AirportsDat,   // 14 fields: the OpenFlights data file
    SavedTree,     // 7 fields, or an empty file (an empty tree was saved)
    Unknown
};
FileKind detectFileKind(const std::string& path);

// One line of a saved tree: code first, text quoted ("" for a "), numbers with the fewest
// digits (15-17) that read back as exactly the same double.
std::string formatSavedLine(const Airport& airport);

// SAVE: writes formatSavedLine for each record, in the given order (preorder).
// Returns false if the file cannot be written.
bool saveAirports(const std::string& path, const std::vector<Airport>& records);

// LOAD-TREE steps 1-6: reads a saved tree into records, in file order.
// Returns 0 on success, -1 if the file cannot be opened, else the 1-based number of the
// first line that is not a saved airport (records is then empty).
int readSavedAirports(const std::string& path, std::vector<Airport>& records);

#endif
