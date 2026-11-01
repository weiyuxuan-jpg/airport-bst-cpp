# Airport Code Lookup Service with a Binary Search Tree

A command-line lookup service for airports, keyed by their three-letter IATA codes (BOS,
JFK, DAC, …). It is built on a binary search tree written from scratch in C++17 and loaded
with the real OpenFlights airport data: 6,072 airports.

Mini project for EECE 2560, Fundamentals of Engineering Algorithms (Northeastern University,
Fall 2026).

- `find` a code and see the search path and the number of comparisons
- `add` and `remove` airports (all three delete cases), then `validate` the tree
- `list` every airport in code order, or a `range` of codes, skipping subtrees outside it
- `save` the tree in preorder and `load` it back with exactly the same shape
- `rebalance` the tree, and an `experiment` comparing four insertion orders

A 48-second screen recording of a session (`find`, `range`, removing a node with two
children, `validate`) is in [docs/demo.mov](docs/demo.mov); [docs/demo.md](docs/demo.md)
says how it was made.

## Build and run

```
g++ -std=c++17 -O2 src/*.cpp -o airports
./airports data/airports.dat
```

Type `help` for the commands and `quit` to leave. A file of commands can be piped in, and the
commands are then echoed so the output reads like a session:

```
./airports < tests/sessions/sample.txt
```

Tested on macOS 26.6 (Apple M5) with Apple clang 21.0.0. It has not been tested on other
systems. `src/main.cpp` uses the POSIX function `isatty`, so on Windows use WSL or MinGW.

On a Mac whose Command Line Tools contain a newer SDK than the linker supports, linking can
fail with `tapi error: … unknown architecture`. Pointing the build at the matching SDK fixes
it, for example:

```
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.sdk g++ -std=c++17 -O2 src/*.cpp -o airports
```

## Sample session

`./airports < tests/sessions/sample.txt` runs the sample session of the project description.
This is its real output. The only change is the cut in the middle of the `range` list, marked
with `[...]`.

```
> load data/airports.dat
Loaded 6072 airports (1626 lines skipped: no IATA code). Duplicates: 0
Height: 177   Average depth: 18.3
> find BOS
Path: GKA -> AEY -> EGS -> BUJ -> BJA -> BSK -> BOY -> BNI -> BOH -> BOO -> BOU -> BOS   (comparisons: 12)
BOS  General Edward Lawrence Logan International Airport, Boston, United States  [ICAO KBOS, lat 42.3643, lon -71.0052]
> find XYZ
Path: GKA -> MAG -> POM -> WWK -> YAM -> XFW -> XGN -> XLS -> XRY -> XVS -> YAI -> YAA -> XYA -> XYE -> (NULL)   not found (comparisons: 14)
> range BOS BWI
113 airports, 127 nodes visited
BOS  General Edward Lawrence Logan International Airport, Boston, United States
BOU  Bourges Airport, Bourges, France
BOW  Bartow Municipal Airport, Bartow, United States
[... 107 more airports, one per line ...]
BWG  Bowling Green Warren County Regional Airport, Bowling Green, United States
BWH  Butterworth Airport, Butterworth, Malaysia
BWI  Baltimore/Washington International Thurgood Marshall Airport, Baltimore, United States
> remove BOS
Deleted BOS (case D2: only a left child, BOR)   validate: OK
> rebalance
Height: 12   Average depth: 10.7
> quit
```

Every number agrees with the expected results in the project description. A few more
commands, run in this order right after `./airports data/airports.dat` (the two `load`
lines at the start are left out):

```
> add QQQ Yuxuan Wei International Airport, Boston, United States
Added QQQ  Yuxuan Wei International Airport, Boston, United States (depth 21)   validate: OK
> remove AEY
Deleted AEY (case D3: two children, successor AFA)   validate: OK
> levels
Level 0: GKA
Level 1: AFA MAG
Level 2: AAE EGS HGU POM
> save tests/bin/tree.txt
Saved 6072 airports in preorder to tests/bin/tree.txt
> load tests/bin/tree.txt
Rebuilt the tree from 6072 airports in preorder (tests/bin/tree.txt)
Height: 177   Average depth: 18.3
validate: OK
```

## Commands

| Command | What it does |
|---|---|
| `load FILE` | `airports.dat`: read it and build the tree in file order. A file written by `save`: rebuild exactly the saved tree. A failed load keeps the old tree. |
| `find CODE` | the airport, the search path and the number of comparisons |
| `min`, `max` | the smallest and the largest code |
| `add CODE NAME, CITY, COUNTRY[, ICAO, LAT, LON]` | insert an airport; a name with a comma goes in quotes |
| `remove CODE` | delete an airport and report the case (D1 leaf, D2 one child, D3 two children) |
| `list` | every airport in code order (inorder), then whether the codes are strictly increasing |
| `levels [k] [CODE]` | k levels (default 3) of the tree level by level, from the root or from CODE; `-` marks an empty position |
| `range FROM TO` | every airport with FROM ≤ code ≤ TO, and how many nodes were visited |
| `validate` | check the BST property with the (lo, hi) range method |
| `save FILE` | write the tree in preorder, one airport per line |
| `rebalance` | rebuild the tree with the middle rule: height ⌊log₂ n⌋ |
| `stats` | number of airports, height and average depth |
| `experiment [FILE]` | the shape experiment below (the current tree is not changed) |
| `help`, `quit` | |

Codes may be typed in lower case. Lines starting with `#` are ignored, which allows comments
in command files.

## How it works

**Classes.**
- `Airport` (`src/Airport.h`) holds one record: name, city, country, IATA code, ICAO code,
  latitude and longitude.
- `AirportBST` (`src/AirportBST.h`) is the tree. `root` and the node type are private, the
  tree owns its nodes and cannot be copied, and every operation keeps the BST property.

The other files hold no classes:
- `src/AirportLoader.cpp` reads and writes the files;
- `src/CommandLoop.cpp` is the text interface;
- `src/ShapeExperiment.cpp` is the experiment.

**Pseudocode.** Every operation was written as pseudocode first, in the notation of the course
notes, and the C++ comments use the same step numbers: [docs/pseudocode.md](docs/pseudocode.md).

**Conventions.**
- Height counts edges: an empty tree has height −1, one node has height 0, and the root has
  depth 0.
- Codes are compared as strings, byte by byte.
- A comparison is one node on the search path. A range query counts every non-empty node it
  enters.

**Deep trees.** Inserting sorted codes gives a chain 6,071 levels deep, so recursion is used
only where its depth is safe:
- `insert`, `find` and `remove` are `while` loops;
- `list`, `save`, `rebalance` and freeing the tree use an explicit stack;
- the height uses a queue;
- `validate`, `range` and the preorder rebuild are recursive, as in the course notes, and are
  tested on the 6,071-level chain.

**Delete.**
- D1, a leaf: the parent's link becomes empty.
- D2, one child: the child takes the node's place.
- D3, two children: the inorder successor's record is copied into the node, and then the
  successor is deleted.

Deleting the root updates `root`.

**Save and load.** `save` writes the airports in preorder, one per line: the code first, then
the rest of the record, quoted like `airports.dat`. For example:

```
"BOS","General Edward Lawrence Logan International Airport","Boston","United States","KBOS",42.36429977,-71.00520325
```

`load` rebuilds the tree from such a file in O(n) by giving each code an allowed (lo, hi)
range. The result has the same shape as the saved tree; the tests compare the full level
order. A file that is not the preorder of a BST is rejected.

## The data

`data/airports.dat` is the OpenFlights airport database, in the version posted on the course
page; see [data/README.md](data/README.md).

Each line has 14 comma-separated fields, with text in double quotes and `\N` for a missing
value. The loader splits each line itself, because two things would break a simple split at
commas:
- a comma inside quotes is text (`"Harstad/Narvik Airport, Evenes"`, 20 lines);
- `""` inside quotes stands for one `"` (`"Magdeburg ""City"" Airport"`, 57 lines).

Only lines whose IATA field is `\N` are skipped. That keeps `DU9` (Dunnville, Canada), the
one code that contains a digit.

The loader's counts match the facts given in the project description:

| | Loader | Project description |
|---|---:|---:|
| Lines | 7,698 | 7,698 |
| Airports with an IATA code | 6,072 | 6,072 |
| Lines skipped (`\N`) | 1,626 | |
| Duplicate codes | 0 | 0 |
| Countries | 235 | 235 |
| Airports in the United States | 1,251 | 1,251 |

## The shape experiment (Task 6)

The same 6,072 records are built into a tree in four orders, and the same 10,000 random
searches are timed on each tree. The method, the three runs and the full explanation are in
[docs/shape_experiment.md](docs/shape_experiment.md). Reproduce with
`printf 'experiment\nquit\n' | ./airports`.

| Build order | Height | Average depth | Time for 10,000 searches | Comparisons per search | Explanation |
|---|---:|---:|---:|---:|---|
| File order | 177 | 18.32 | 1.01 – 1.17 ms | 19.3 | The 181 Canadian airports on lines 21–205 are listed in ICAO order, and their IATA code is the ICAO code without the "C", so their codes arrive sorted. A run of 162 increasing codes (YDA … ZFM) becomes a chain of right children; 155 of them lie on the deepest path. |
| Sorted order | 6,071 | 3,035.5 | 109 – 130 ms | 3,007.7 | Every new code is larger than all before it, so every insert goes right: a chain of n − 1 edges, the worst case. |
| Balanced (middle rule) | 12 | 10.65 | 0.61 – 0.65 ms | 11.7 | The middle element is the root of each part, so all levels except the last are full: ⌊log₂ 6072⌋ = 12. |
| Random shuffle (bonus) | 26 | 14.2 | 0.86 – 0.91 ms | 15.2 | No long increasing runs (the longest is 7). 100 seeds give heights 24–34 (median 28) and a mean average depth of 14.57, close to the expected 14.58 for a random order. |

- **How the times were measured.** Each time is the median of 7 timed runs; the table gives
  its range over three runs of the program. Machine: Apple M5, `-O2`.
- **The stable measure.** The comparison counts do not depend on the machine. They follow
  the depth: about average depth + 1.

**Why the file order is so much worse than a random order.** The lines of `airports.dat` are
sorted by airport ID, and the IDs were given country by country. Lines 21–205 are Canada in
ICAO order. For Canadian airports the IATA code is the ICAO code without the leading C
(CYDA → YDA), so those codes arrive in increasing order.

Inserting increasing codes that all fall into one gap of the tree makes each one the right
child of the one before. The deepest path, to YZR, is made of:
- the 4 first codes of the file (Papua New Guinea);
- 19 Canadian codes from lines 21–41;
- 155 codes of that run, the last 154 of them all steps to the right.

Inserts never move existing nodes, so later inserts cannot shorten this chain. A shuffle
breaks such runs (57.8% of neighbouring codes increase in the file, 50.5% after a shuffle).
That is why random orders give heights of only 24–34 (median 28 over 100 seeds).

## Tests

```
sh tests/run_tests.sh              # all test programs, -O2
SANITIZE=1 sh tests/run_tests.sh   # the same with AddressSanitizer and UndefinedBehaviorSanitizer
```

(On the Mac described above, set `SDKROOT` first.) There are 13 test programs with 626 checks;
all pass. Among other things they check:
- the loader against Python's `csv` module, every field of every record (done once, outside
  the test suite);
- every search case;
- every delete case, with the new depth of every node predicted, and all 6,072 codes deleted
  in random order with `validate` after each one;
- trees the lecture shows are not BSTs, which `validate` must reject;
- `range` against a brute-force filter on 500 random ranges, with the visited count predicted
  exactly;
- save/load round trips, and damaged save files;
- every claim of the explanation above.

Bugs were planted on purpose in copies of the code to make sure the tests catch them.
`tests/compile_fail/private_members.cpp` must fail to compile: code outside the tree cannot
reach `root` or the nodes.

## Project layout

```
src/        Airport, AirportBST, loader and save files, command loop, experiment, main
data/       airports.dat (OpenFlights) and its credit
docs/       pseudocode.md, shape_experiment.md, demo.mov (screen recording), demo.md (how it is made)
tests/      test programs, run_tests.sh, sessions/ (command files: sample, demo, ...)
LICENSE     MIT license for the code
```

## License

The code is released under the [MIT License](LICENSE).

The airport data in `data/airports.dat` is not covered by the MIT License. It stays under
the OpenFlights licenses (Open Database License and Database Contents License); see
[data/README.md](data/README.md).

## Credits

Airport data: **OpenFlights.org** (https://openflights.org/data.php). The database is made
available under the [Open Database License](https://opendatacommons.org/licenses/odbl/1-0/), and
its individual contents under the
[Database Contents License](http://opendatacommons.org/licenses/dbcl/1.0/). See
[data/README.md](data/README.md).

Author: Yuxuan Wei.
