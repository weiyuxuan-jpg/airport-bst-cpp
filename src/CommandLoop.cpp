#include "CommandLoop.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "Airport.h"
#include "AirportBST.h"
#include "AirportLoader.h"
#include "ShapeExperiment.h"

namespace {

std::string toUpper(std::string text) {
    int n = static_cast<int>(text.size());
    for (int i = 0; i < n; i++) text[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
    return text;
}

std::string toLower(std::string text) {
    int n = static_cast<int>(text.size());
    for (int i = 0; i < n; i++) text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    return text;
}

std::string trim(const std::string& text) {
    std::string::size_type first = text.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    std::string::size_type last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

// Text after the first occurrence of word, without surrounding spaces
// (a file name or an airport name may contain spaces).
std::string restOfLine(const std::string& line, const std::string& word) {
    std::string::size_type start = line.find(word);
    if (start == std::string::npos) return "";
    return trim(line.substr(start + word.size()));
}

void printShape(const AirportBST& tree, std::ostream& out) {
    out << "Height: " << tree.height() << "   Average depth: " << std::fixed
        << std::setprecision(1) << tree.averageDepth() << "\n";
    out.unsetf(std::ios::fixed);
}

void printHelp(std::ostream& out) {
    out << "Commands:\n"
           "  load FILE        airports.dat: build the tree in file order;\n"
           "                   a saved tree: rebuild exactly the saved shape\n"
           "  save FILE        write the tree in preorder (code first, then the record)\n"
           "  find CODE        search path, comparisons and the airport\n"
           "  min | max        smallest / largest code\n"
           "  add CODE NAME, CITY, COUNTRY[, ICAO, LAT, LON]\n"
           "                   insert an airport (quote a name that contains a comma)\n"
           "  remove CODE      delete an airport (cases D1, D2, D3)\n"
           "  list             every airport in code order (inorder), then the sorted check\n"
           "  levels [k] [CODE]  k levels (1-6, default 3) from the root, or from CODE\n"
           "  range FROM TO    every airport with FROM <= code <= TO, and the nodes visited\n"
           "  validate         check the BST property with (lo, hi) ranges\n"
           "  rebalance        rebuild the tree with the middle rule (height floor(log2 n))\n"
           "  experiment [FILE]  the shape experiment on FILE (default data/airports.dat);\n"
           "                   the current tree is not changed\n"
           "  stats            number of airports, height and average depth\n"
           "  help             this list\n"
           "  quit             leave\n";
}

// load of an airports.dat file: LOAD-AIRPORTS + BUILD-FROM-ARRAY (sample session lines 1-3).
void loadAirportsDat(AirportBST& tree, const std::string& path, std::ostream& out) {
    std::vector<Airport> records;
    LoadReport load;
    if (!loadAirports(path, records, load)) {
        out << "Error: cannot open " << path << " (the tree is unchanged)\n";
        return;
    }
    tree.clear();
    BuildReport build = tree.buildFromArray(records);
    out << "Loaded " << build.inserted << " airports (" << load.skipped
        << " lines skipped: no IATA code). Duplicates: " << build.duplicates.size() << "\n";
    int dups = static_cast<int>(build.duplicates.size());
    for (int i = 0; i < dups; i++) out << "  duplicate code: " << build.duplicates[i] << "\n";
    if (load.malformed > 0) {
        out << "  malformed lines: " << load.malformed << " (first: line " << load.malformedLines[0] << ")\n";
    }
    printShape(tree, out);
}

// load of a saved tree: LOAD-TREE (B3 rebuild of the saved shape).
void loadSavedTree(AirportBST& tree, const std::string& path, std::ostream& out) {
    std::vector<Airport> list;
    int bad = readSavedAirports(path, list);
    if (bad < 0) {
        out << "Error: cannot open " << path << " (the tree is unchanged)\n";
        return;
    }
    if (bad > 0) {
        out << "Error: line " << bad << " of " << path << " is not a saved airport (7 fields: code, "
            << "name, city, country, ICAO, latitude, longitude); the tree is unchanged\n";
        return;
    }
    int stoppedAt = -1;
    if (!tree.buildFromPreorder(list, stoppedAt)) {
        out << "Error: " << path << " is not the preorder of a BST: line " << stoppedAt + 1 << " ("
            << list[stoppedAt].getIata() << ") does not fit; the tree is unchanged\n";
        return;
    }
    out << "Rebuilt the tree from " << list.size() << " airports in preorder (" << path << ")\n";
    printShape(tree, out);
    out << "validate: " << tree.validate().describe() << "\n";
}

void cmdLoad(AirportBST& tree, const std::string& path, std::ostream& out) {
    if (path.empty()) {
        out << "Usage: load FILE\n";
        return;
    }
    switch (detectFileKind(path)) {
        case FileKind::CannotOpen:
            out << "Error: cannot open " << path << " (the tree is unchanged)\n";
            return;
        case FileKind::AirportsDat:
            loadAirportsDat(tree, path, out);
            return;
        case FileKind::SavedTree:
            loadSavedTree(tree, path, out);
            return;
        case FileKind::Unknown:
            out << "Error: " << path << " is neither airports.dat (14 fields per line) nor a saved "
                << "tree (7 fields per line); the tree is unchanged\n";
            return;
    }
}

void cmdSave(const AirportBST& tree, const std::string& path, std::ostream& out) {
    if (path.empty()) {
        out << "Usage: save FILE\n";
        return;
    }
    std::vector<Airport> list = tree.preorder();
    if (!saveAirports(path, list)) {
        out << "Error: cannot write " << path << "\n";
        return;
    }
    out << "Saved " << list.size() << " airports in preorder to " << path << "\n";
}

// Prints in the format of the sample session:
//   Path: GKA -> ... -> BOS   (comparisons: 12)
//   Path: GKA -> ... -> (NULL)   not found (comparisons: 14)
void cmdFind(const AirportBST& tree, const std::string& code, std::ostream& out) {
    if (code.empty()) {
        out << "Usage: find CODE\n";
        return;
    }
    FindResult r = tree.find(code);
    out << "Path: ";
    int n = r.comparisons();
    for (int i = 0; i < n; i++) {
        if (i > 0) out << " -> ";
        out << r.path[i];
    }
    if (r.found) {
        out << "   (comparisons: " << n << ")\n";
        out << r.airport.details() << "\n";
    } else {
        out << (n > 0 ? " -> " : "") << "(NULL)   not found (comparisons: " << n << ")\n";
    }
}

void cmdMinMax(const AirportBST& tree, bool wantMinimum, std::ostream& out) {
    Airport airport;
    bool ok = wantMinimum ? tree.minimum(airport) : tree.maximum(airport);
    if (!ok) {
        out << "The tree is empty.\n";
        return;
    }
    out << (wantMinimum ? "Minimum: " : "Maximum: ") << airport.summary() << "\n";
}

// ADD (docs/pseudocode.md, section 12). line is the whole command line.
void cmdAdd(AirportBST& tree, const std::string& line, const std::string& command,
            const std::string& codeWord, std::ostream& out) {
    const char* usage = "Usage: add CODE NAME, CITY, COUNTRY[, ICAO, LAT, LON]\n";
    std::string code = toUpper(codeWord);                                     // step 1
    std::string rest = restOfLine(restOfLine(line, command), codeWord);
    bool codeOk = code.size() == 3;                                           // step 2
    int length = static_cast<int>(code.size());
    for (int i = 0; i < length; i++) {
        unsigned char c = static_cast<unsigned char>(code[i]);
        if (!std::isupper(c) && !std::isdigit(c)) codeOk = false;
    }
    if (codeWord.empty()) {
        out << usage;
        return;
    }
    if (!codeOk) {
        out << "Bad code '" << codeWord << "': a code has 3 letters or digits.\n";
        return;
    }

    std::vector<std::string> f = splitCsvLine(rest);                          // step 3
    int n = static_cast<int>(f.size());
    for (int i = 0; i < n; i++) f[i] = trim(f[i]);
    if ((n != 3 && n != 6) || f[0].empty() || f[1].empty() || f[2].empty()) {  // step 4
        out << usage;
        return;
    }
    std::string icao;                                                         // step 5
    double latitude = 0.0;
    double longitude = 0.0;
    if (n == 6) {                                                             // step 6
        icao = f[3];                                                          // step 7
        if (!parseCoordinates(f[4], f[5], latitude, longitude)) {             // steps 8-9
            out << "Bad coordinates '" << f[4] << ", " << f[5]
                << "': latitude -90 to 90, longitude -180 to 180.\n";
            return;
        }
    }

    Airport airport(f[0], f[1], f[2], code, icao, latitude, longitude);
    if (tree.insert(airport)) {                                               // step 10
        out << "Added " << airport.summary() << " (depth "                    // step 11
            << tree.find(code).comparisons() - 1 << ")   validate: "
            << tree.validate().describe() << "\n";
    } else {
        out << code << " is already in the tree (rule I4); nothing was added.\n";   // step 12
    }
}

// "Deleted BOS (case D2: only a left child, BOR)   validate: OK", as in the sample session.
void cmdRemove(AirportBST& tree, const std::string& code, std::ostream& out) {
    if (code.empty()) {
        out << "Usage: remove CODE\n";
        return;
    }
    RemoveResult r = tree.remove(code);
    if (r.kind == DeleteCase::NotFound) {
        out << code << " is not in the tree.\n";
        return;
    }
    out << "Deleted " << code << " (" << r.describe() << ")";
    if (r.wasRoot) {
        std::vector<NodeInfo> nodes = tree.nodes();
        out << "; " << code << " was the root, new root: " << (nodes.empty() ? "(empty tree)" : nodes[0].code);
    }
    out << "   validate: " << tree.validate().describe() << "\n";
}

void cmdValidate(const AirportBST& tree, std::ostream& out) {
    ValidateResult v = tree.validate();
    out << "validate: " << v.describe();
    if (v.ok) out << " (" << v.reached << " nodes, each inside its (lo, hi) range)";
    out << "\n";
}

void cmdList(const AirportBST& tree, std::ostream& out) {
    std::vector<Airport> list = tree.inorder();
    int n = static_cast<int>(list.size());
    for (int i = 0; i < n; i++) out << list[i].summary() << "\n";
    int bad = -1;
    bool sorted = isSorted(list, bad);
    out << n << " airports listed in code order; sorted: " << (sorted ? "yes" : "NO");
    if (!sorted) out << " (" << list[bad].getIata() << " >= " << list[bad + 1].getIata() << ")";
    out << "\n";
}

// levels [k] [CODE]: "Level d: ..." for k levels, "-" for an empty position.
void cmdLevels(const AirportBST& tree, const std::vector<std::string>& args, std::ostream& out) {
    int k = 3;
    std::string code;
    int n = static_cast<int>(args.size());
    for (int i = 0; i < n; i++) {
        bool number = !args[i].empty() && args[i].size() <= 2 &&
                      args[i].find_first_not_of("0123456789") == std::string::npos;
        if (number) k = std::stoi(args[i]);
        else code = toUpper(args[i]);
    }
    if (k < 1 || k > 6) {
        out << "Usage: levels [k] [CODE]   (k from 1 to 6, default 3)\n";
        return;
    }
    if (tree.isEmpty()) {
        out << "The tree is empty.\n";
        return;
    }
    int startDepth = 0;
    if (!code.empty()) {
        FindResult f = tree.find(code);
        if (!f.found) {
            out << code << " is not in the tree.\n";
            return;
        }
        startDepth = f.comparisons() - 1;
        out << "Levels from " << code << ":\n";
    }
    std::vector<std::string> rows = tree.topLevels(k, code);
    int m = static_cast<int>(rows.size());
    for (int i = 0; i < m; i++) out << "Level " << startDepth + i << ": " << rows[i] << "\n";
}

// "113 airports, 127 nodes visited" (sample session line 10), then one line per airport.
void cmdRange(const AirportBST& tree, const std::vector<std::string>& args, std::ostream& out) {
    if (args.size() != 2) {
        out << "Usage: range FROM TO\n";
        return;
    }
    std::string from = toUpper(args[0]);
    std::string to = toUpper(args[1]);
    if (from > to) {
        out << "FROM must not come after TO (" << from << " > " << to << ").\n";
        return;
    }
    RangeResult r = tree.range(from, to);
    int n = static_cast<int>(r.airports.size());
    out << n << " airports, " << r.visited << " nodes visited\n";
    for (int i = 0; i < n; i++) out << r.airports[i].summary() << "\n";
}

// "Height: 12   Average depth: 10.7" (sample session line 14).
void cmdRebalance(AirportBST& tree, std::ostream& out) {
    tree.rebalance();
    printShape(tree, out);
}

// SHAPE-EXPERIMENT on the records of an airports.dat file (10,000 searches, 7 timed runs).
void cmdExperiment(const std::string& pathArgument, std::ostream& out) {
    const int queries = 10000;
    const int repeats = 7;
    const unsigned seed = 2560;
    std::string path = pathArgument.empty() ? "data/airports.dat" : pathArgument;
    std::vector<Airport> records;
    LoadReport load;
    if (!loadAirports(path, records, load)) {
        out << "Error: cannot open " << path << "\n";
        return;
    }
    out << "Shape experiment on " << records.size() << " airports from " << path
        << " (the current tree is not changed)\n";
    std::vector<ShapeRow> rows = runShapeExperiment(records, queries, repeats, seed);
    printShapeTable(rows, queries, repeats, seed, out);
    out << "(Random shuffle uses seed " << seed << ".)\n";
    printSeedSweep(sweepSeeds(records, 1, 100), 1, out);
}

void cmdStats(const AirportBST& tree, std::ostream& out) {
    out << "Airports: " << tree.size() << "   ";
    printShape(tree, out);
}

// Runs one command line; returns false for quit.
bool execute(AirportBST& tree, const std::string& line, std::ostream& out) {
    std::istringstream words(line);
    std::string command;
    if (!(words >> command)) return true;              // empty line
    if (command[0] == '#') return true;                // comment line in a script
    std::vector<std::string> args;
    std::string word;
    while (words >> word) args.push_back(word);
    std::string argument = args.empty() ? "" : args[0];
    std::string name = toLower(command);

    if (name == "quit" || name == "exit") return false;
    if (name == "load") cmdLoad(tree, restOfLine(line, command), out);
    else if (name == "save") cmdSave(tree, restOfLine(line, command), out);
    else if (name == "find") cmdFind(tree, toUpper(argument), out);
    else if (name == "min") cmdMinMax(tree, true, out);
    else if (name == "max") cmdMinMax(tree, false, out);
    else if (name == "add") cmdAdd(tree, line, command, argument, out);
    else if (name == "remove") cmdRemove(tree, toUpper(argument), out);
    else if (name == "list") cmdList(tree, out);
    else if (name == "levels") cmdLevels(tree, args, out);
    else if (name == "range") cmdRange(tree, args, out);
    else if (name == "validate") cmdValidate(tree, out);
    else if (name == "rebalance") cmdRebalance(tree, out);
    else if (name == "experiment") cmdExperiment(restOfLine(line, command), out);
    else if (name == "stats") cmdStats(tree, out);
    else if (name == "help") printHelp(out);
    else out << "Unknown command '" << command << "'. Type help for the list.\n";
    return true;
}

}  // namespace

int runCommandLoop(std::istream& in, std::ostream& out, bool echo,
                   const std::vector<std::string>& startup) {
    AirportBST tree;
    int n = static_cast<int>(startup.size());
    for (int i = 0; i < n; i++) {
        out << "> " << startup[i] << "\n";
        if (!execute(tree, startup[i], out)) return 0;
    }
    std::string line;
    while (true) {
        out << "> " << std::flush;
        if (!std::getline(in, line)) {
            out << "\n";
            break;
        }
        if (echo) out << line << "\n";
        if (!execute(tree, line, out)) break;
    }
    return 0;
}
