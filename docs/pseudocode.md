# Pseudocode — Airport Code Lookup Service (BST)

Main steps of every operation, written before the C++ code, in the notation of the course
lecture notes (Lecture 07, Binary Search Trees). The rule labels (P, I, S, D, B) are the
lecture's; the step numbers here are the ones the C++ comments will refer to.

| Section | Operation | Project task |
|---|---|---|
| [0](#0-conventions) | Conventions used by every operation | all |
| [1](#1-load-the-data-file) | `SPLIT-CSV-LINE`, `LOAD-AIRPORTS` | Task 1(a) |
| [2](#2-insert-and-build-in-file-order) | `INSERT`, `BUILD-FROM-ARRAY` | Task 1(b) |
| [3](#3-height-and-average-depth) | `HEIGHT-AND-DEPTH` | Task 1(c), Task 6 |
| [4](#4-search-minimum-maximum) | `FIND`, `MINIMUM`, `MAXIMUM` | Task 2 |
| [5](#5-delete) | `REMOVE` (cases D1, D2, D3) | Task 3 |
| [6](#6-traversals) | `INORDER`, `IS-SORTED`, `LEVEL-ORDER`, `TOP-LEVELS` | Task 3(b), Task 4(a) |
| [7](#7-validate) | `IS-BST`, `VALIDATE` | Task 4(c) |
| [8](#8-save-and-load-in-preorder) | `SAVE`, `LOAD-TREE`, `BUILD-FROM-PREORDER` | Task 4(b) |
| [9](#9-range-query) | `RANGE` | Task 5 |
| [10](#10-balanced-build-and-rebalance) | `BUILD-BALANCED`, `REBALANCE` | Task 6 |
| [11](#11-shape-experiment) | `SHAPE-EXPERIMENT` | Task 6 |
| [12](#12-command-loop) | Commands | all |
| [13](#13-cost-summary) | Cost of each operation | all |

---

## 0. Conventions

**Notation.** `←` is assignment, `NULL` is an empty pointer, `NONE` is "no bound" (−∞ or +∞).
`for i ← 0 to n − 1 do` is written in C++ as `for (int i = 0; i < n; i++)`, as the project asks.

**Data.**

| Type | Fields | Role |
|---|---|---|
| `Airport` | name, city, country, iata, icao, latitude, longitude | one record; `iata` is the key |
| `Node` (private to the tree) | data : Airport, left, right | one tree node |
| `AirportBST` | root (private), count | the tree; owns every node |

In the pseudocode `node.key` means `node.data.iata`.

**Key comparison.** Keys are compared as strings, byte by byte (`std::string` `<`, `==`).
All codes are 3 upper-case ASCII characters, so this is alphabetical order; the one code
with a digit, `DU9`, sorts before `DUA` because `'9'` < `'A'`. Codes typed by the user are
converted to upper case first.

**Distinct keys (I4).** A key that is already in the tree is a duplicate: it is not inserted,
and the build reports it.

**Height and depth.** The depth of the root is 0. Height counts edges: empty tree −1, one node
0. Average depth = (sum of all node depths) / (number of nodes).

**Counting.**
- *Comparisons* in `FIND` = number of nodes on the search path. Each node is compared with
  the target once (one three-way comparison), as in the lecture ("3 comparisons, one per level").
- *Nodes visited* in `RANGE` = number of non-NULL nodes the function enters.

**Loops or recursion.** Inserting sorted keys makes a chain of 6,071 nodes (P4), so every
operation that follows a path or touches every node can go 6,071 levels deep.

| Operation | Written as | Why |
|---|---|---|
| `INSERT`, `FIND`, `MINIMUM`, `MAXIMUM`, `REMOVE` | `while` loop | the project note: recursion may overflow the call stack on the sorted chain |
| `HEIGHT-AND-DEPTH`, `LEVEL-ORDER`, `TOP-LEVELS` | queue | breadth-first, no recursion needed |
| `INORDER`, `SAVE` (preorder), clear | explicit stack | works for any shape; `REBALANCE` may be called on the chain |
| `IS-BST`, `RANGE`, `BUILD-FROM-PREORDER` | recursion, depth ≤ h + 1 | as in the lecture; depth 178 on the file-order tree, to be tested on the 6,071 chain in Task 6 |
| `BUILD-BALANCED` | recursion, depth ≤ ⌊log₂ n⌋ + 1 = 13 | each call halves the range |

---

## 1. Load the data file

`airports.dat` has one airport per line and 14 comma-separated fields. Text is in double
quotes, a missing value is `\N`. Two traps:

- a comma inside quotes is part of the text: `"Harstad/Narvik Airport, Evenes"` (20 lines);
- a doubled quote inside quotes is one quote character: `"Magdeburg ""City"" Airport"` (57 lines).

So a line is split with a small state machine instead of "split at every comma".

### SPLIT-CSV-LINE(line): main steps

```
SPLIT-CSV-LINE(line)                          // returns the list of fields
 1. fields ← empty list; cur ← ""; inQuotes ← FALSE; n ← length(line)
 2. for i ← 0 to n − 1 do
 3.     c ← line[i]
 4.     if inQuotes then
 5.         if c = '"' and i + 1 < n and line[i + 1] = '"' then
 6.             append '"' to cur; i ← i + 1         // "" inside quotes → one "
 7.         else if c = '"' then inQuotes ← FALSE    // closing quote
 8.         else append c to cur                     // includes a comma inside quotes
 9.     else
10.         if c = '"' then inQuotes ← TRUE          // opening quote
11.         else if c = ',' then append cur to fields; cur ← ""   // field ends
12.         else append c to cur
13. append cur to fields                             // the last field
14. return fields
```

Trace on the start of line 329, `332,"Magdeburg ""City"" Airport","Magdeburg",…`:

| Characters read | State after | cur | fields |
|---|---|---|---|
| `332` | outside | `332` | |
| `,` | outside | (empty) | `332` |
| `"Magdeburg ` | inside | `Magdeburg ` | `332` |
| `""` (step 6) | inside | `Magdeburg "` | `332` |
| `City""` | inside | `Magdeburg "City"` | `332` |
| ` Airport"` (step 7 closes) | outside | `Magdeburg "City" Airport` | `332` |
| `,` | outside | (empty) | `332`, `Magdeburg "City" Airport` |

### LOAD-AIRPORTS(path): main steps

Decision (2026-10-21): only lines whose IATA field is `\N` are skipped, so `DU9` is kept.

```
LOAD-AIRPORTS(path)                           // returns the vector of Airport records
 1. open path; if it cannot be opened then report the error; return an empty vector
 2. records ← empty vector; lines ← 0; skipped ← 0; malformed ← 0
 3. while a line can be read from the file do
 4.     lines ← lines + 1
 5.     if line ends with '\r' then remove it        // file saved on Windows
 6.     f ← SPLIT-CSV-LINE(line)
 7.     if size(f) ≠ 14 then malformed ← malformed + 1; continue
 8.     if f[4] = "\N" then skipped ← skipped + 1; continue      // no IATA code
 9.     if length(f[4]) ≠ 3 or f[6], f[7] are not coordinates then   // finite, |lat| ≤ 90, |lon| ≤ 180
10.         malformed ← malformed + 1; continue
11.     icao ← "" if f[5] = "\N", else f[5]
12.     append Airport(f[1], f[2], f[3], f[4], icao, number(f[6]), number(f[7])) to records
13. report lines, size(records), skipped, malformed
14. return records
```

Expected for the posted file: 7,698 lines, 6,072 records, 1,626 skipped, 0 malformed.

---

## 2. Insert and build in file order

### INSERT(airport): main steps

As in the lecture (Section 2.4), but it returns whether a node was added so the build can
count duplicates.

```
INSERT(airport)                               // returns TRUE if added, FALSE if duplicate
 1. key ← airport.iata
 2. if root = NULL then root ← new node(airport); count ← count + 1; return TRUE   // I1
 3. cur ← root
 4. loop
 5.     if key < cur.key then                                                       // I2
 6.         if cur.left = NULL then cur.left ← new node(airport); count ← count + 1; return TRUE
 7.         cur ← cur.left
 8.     else if key > cur.key then                                                  // I3
 9.         if cur.right = NULL then cur.right ← new node(airport); count ← count + 1; return TRUE
10.         cur ← cur.right
11.     else return FALSE                                                           // I4 duplicate
```

The node is created only at the NULL spot, so a duplicate allocates nothing.

### BUILD-FROM-ARRAY(records, n): main steps (method B1)

```
BUILD-FROM-ARRAY(records, n)                  // insert in file order
1. inserted ← 0; duplicates ← 0
2. for i ← 0 to n − 1 do
3.     if INSERT(records[i]) then inserted ← inserted + 1
4.     else duplicates ← duplicates + 1; report records[i].iata as a duplicate    // I4
5. report inserted, duplicates
```

Expected: 6,072 inserted, 0 duplicates.

---

## 3. Height and average depth

One breadth-first pass gives both numbers. Each queue entry carries the node's depth.

### HEIGHT-AND-DEPTH(): main steps

```
HEIGHT-AND-DEPTH()                            // returns (height, average depth)
 1. if root = NULL then return (−1, 0)
 2. Q ← empty queue; enqueue (root, 0)
 3. height ← 0; total ← 0
 4. while Q is not empty do
 5.     (node, d) ← dequeue Q
 6.     total ← total + d
 7.     if d > height then height ← d
 8.     if node.left ≠ NULL then enqueue (node.left, d + 1)
 9.     if node.right ≠ NULL then enqueue (node.right, d + 1)
10. return (height, total / count)
```

Expected for the file-order tree: height 177, average depth 18.3.

---

## 4. Search, minimum, maximum

### FIND(target): main steps

The lecture's SEARCH (Section 4.3), keeping the path.

```
FIND(target)                                  // returns (node or NULL, path)
1. path ← empty list; cur ← root
2. while cur ≠ NULL do
3.     append cur.key to path                    // one comparison per node
4.     if target = cur.key then return (cur, path)            // S2 found
5.     else if target < cur.key then cur ← cur.left           // S3
6.     else cur ← cur.right                                   // S4
7. return (NULL, path)                                        // S1 reached NULL: not found
```

Printing, in the format of the sample session (comparisons = size(path)); the program
prints the whole path where the sample shows `…`, and adds the ICAO code and location:

```
found:      Path: GKA -> … -> BOS   (comparisons: 12)
            BOS  General Edward Lawrence Logan International Airport, Boston, United States  [ICAO KBOS, lat 42.3643, lon -71.0052]
not found:  Path: GKA -> … -> XYE -> (NULL)   not found (comparisons: 14)
```

In C++ each node is compared once with `std::string::compare`, which gives <, = or > in one
call, so the comparison count is exactly the number of nodes on the path.

Search cases to test (Task 2(b)): the root; a leaf; a deep node; a missing code (`XYZ`);
a code smaller than every key; a code larger than every key. The last two follow the
leftmost / rightmost path and end at NULL.

### MINIMUM() and MAXIMUM(): main steps (P2)

```
MINIMUM()                                     MAXIMUM()
1. if root = NULL then return NULL            1. if root = NULL then return NULL
2. cur ← root                                 2. cur ← root
3. while cur.left ≠ NULL do cur ← cur.left    3. while cur.right ≠ NULL do cur ← cur.right
4. return cur                                 4. return cur
```

---

## 5. Delete

The lecture's three cases (Section 5.1), written with a loop and a `parent` pointer instead of
recursion. That way deleting the root is an explicit step (step 15), and the case can be
reported, as in the sample session's `case D2: only a left child, BOR`.

| Case | Node to delete has | Action |
|---|---|---|
| D1 | no children | parent's pointer becomes NULL |
| D2 | one child (left only, or right only) | the child replaces the node |
| D3 | two children | copy the **inorder successor** (smallest key of the right subtree) into the node, then delete the successor, which has no left child, so it is D1 or D2 |

D4 in the lecture (use the predecessor instead) is equally correct; this project uses the
successor, as the lecture's code does.

### REMOVE(target): main steps

```
REMOVE(target)                                // returns the case, or NOT FOUND
 1. parent ← NULL; cur ← root
 2. while cur ≠ NULL and cur.key ≠ target do             // search, remembering the parent
 3.     parent ← cur
 4.     if target < cur.key then cur ← cur.left else cur ← cur.right
 5. if cur = NULL then return NOT FOUND
 6. if cur.left = NULL and cur.right = NULL then case ← D1
 7. else if cur.right = NULL then case ← (D2, only a left child, cur.left.key)
 8. else if cur.left = NULL then case ← (D2, only a right child, cur.right.key)
 9. else                                                 // D3 two children
10.     sParent ← cur; s ← cur.right
11.     while s.left ≠ NULL do sParent ← s; s ← s.left     // s = inorder successor
12.     case ← (D3, successor s.key)
13.     cur.data ← s.data                                  // copy the successor's record
14.     cur ← s; parent ← sParent                          // now delete s (D1 or D2)
15. child ← cur.left if cur.left ≠ NULL, else cur.right    // NULL for D1
16. if parent = NULL then root ← child                     // deleting the root
17. else if parent.left = cur then parent.left ← child
18. else parent.right ← child
19. free cur; count ← count − 1
20. return case
```

Step 17 compares pointers, not keys: after step 13 two nodes briefly hold the same key.

When the successor is the right child itself (`cur.right` has no left child), step 11 does
not move, so `sParent = cur` and steps 17–18 relink `cur.right`, not a left link. Neither
lecture example has this shape; the tests add it, at the root and below it. A common mistake,
always writing `sParent.left ← s.right`, breaks exactly this case.

Output, as in the sample session: `Deleted BOS (case D2: only a left child, BOR)`. Deleting
the root adds `; GKA was the root, new root: GKE`.

Trace on the lecture's tree with 35 inserted (preorder `40 20 10 30 35 60 50 70`):

| Delete | Steps 1–5 path | Case | Relinking | Preorder after |
|---|---|---|---|---|
| 10 | 40, 20, 10 | D1 | parent 20: left ← NULL | 40 20 30 35 60 50 70 |
| 30 | 40, 20, 30 | D2, only a right child, 35 | parent 20: right ← 35 | 40 20 35 60 50 70 |
| 40 (root) | 40 | D3, successor 50 | copy 50 into the root; s = 50, sParent = 60: 60.left ← NULL | 50 20 35 60 70 |

These are the lecture's results (Section 5.2).

After every delete in the tests: `VALIDATE`, then `FIND` the deleted code (must be not
found), and print `TOP-LEVELS(3)` before and after.

---

## 6. Traversals

### INORDER(): main steps (P1)

Explicit stack, so it also works on the 6,071-node chain.

```
INORDER()                                     // returns the records in key order
1. out ← empty list; S ← empty stack; cur ← root
2. while cur ≠ NULL or S is not empty do
3.     while cur ≠ NULL do push cur on S; cur ← cur.left      // go left as far as possible
4.     cur ← pop S
5.     append cur.data to out                                 // visit
6.     cur ← cur.right
7. return out
```

### IS-SORTED(list, n): main steps

```
IS-SORTED(list, n)                            // strictly increasing codes?
1. for i ← 0 to n − 2 do
2.     if list[i].iata ≥ list[i + 1].iata then return FALSE  // report the pair
3. return TRUE
```

`list` prints `INORDER()`, then checks `IS-SORTED` and that `size(out) = count`.

### LEVEL-ORDER(): main steps

```
LEVEL-ORDER()                                 // returns the codes level by level
1. out ← empty list
2. if root = NULL then return out
3. Q ← empty queue; enqueue root
4. while Q is not empty do
5.     node ← dequeue Q; append node.key to out
6.     if node.left ≠ NULL then enqueue node.left
7.     if node.right ≠ NULL then enqueue node.right
8. return out
```

For a BST, the level-order list fixes the shape: inserting the keys in level order rebuilds
the same tree, because each key comes after all its ancestors. So two BSTs with the same
level-order list have the same shape (used in Task 4(b)). Any order in which every key comes
after its ancestors works the same way; preorder is one (Section 8).

### TOP-LEVELS(k, start): main steps

To show the shape before and after a delete, k levels are printed with `-` for an empty
position. Level d of the view has 2^d positions. `start` is the root, or the node holding a
given code (`levels 3 BOU`), which shows the change for a delete deeper than level 2.

```
TOP-LEVELS(k, start)                          // k levels, starting at start
1. Q ← empty queue; enqueue start (it may be NULL)
2. for level ← 0 to k − 1 do
3.     m ← size(Q)                                    // = 2^level positions
4.     for i ← 0 to m − 1 do
5.         node ← dequeue Q
6.         if node = NULL then add "-" to the row; enqueue NULL; enqueue NULL
7.         else add node.key to the row; enqueue node.left; enqueue node.right
8.     if the row has no key then stop               // nothing at this level or below
9.     print the row
```

Top three levels of the file-order tree (C++ output): `GKA` / `AEY MAG` / `AAE EGS HGU POM`.
A delete changes them only if the deleted node, or a node that moves up, is in levels 0–2.

---

## 7. Validate

Each node gets the open range (lo, hi) allowed by all its ancestors (lecture Section 6,
P5). Checking a node only against its own children is not enough (lecture Figure 3).

### IS-BST(node, lo, hi): main steps

```
IS-BST(node, lo, hi)                          // lo, hi are codes or NONE
1. if node = NULL then return TRUE
2. reached ← reached + 1
3. if (lo ≠ NONE and node.key ≤ lo) or (hi ≠ NONE and node.key ≥ hi) then
4.     remember node.key, lo, hi as the first violation; return FALSE
5. return IS-BST(node.left, lo, node.key) and IS-BST(node.right, node.key, hi)
Initial call: reached ← 0; IS-BST(root, NONE, NONE)
```

`≤` and `≥` in step 3 make a duplicate key fail too (keys are distinct, I4).

### VALIDATE(): main steps

```
VALIDATE()
1. ok ← IS-BST(root, NONE, NONE)
2. ok ← ok and (reached = count)              // catches a lost node or a wrong count
3. return ok, with the first violation or the two counts when it fails
```

Printed as `validate: OK`, or for example
`validate: FAILED (K45 is not in (K20, K40))` and `validate: FAILED (6072 nodes reached, count is 6073)`.
`remove` and `add` print it at the end of their line, as in the sample session:
`Deleted BOS (case D2: only a left child, BOR)   validate: OK`.

Run after the build, after every insert and delete in the tests, and after `LOAD-TREE` and
`REBALANCE`. A hand-made broken tree (the lecture's Figure 3) must give FALSE.

---

## 8. Save and load in preorder

Saving in preorder keeps the shape: the first key is the root, and the keys smaller than it
that follow form its left subtree (lecture B3, Section 3.5 Application 2). No NULL markers
are needed.

**File format (decided 2026-10-21).** One line per node, in preorder: the code first, then the
rest of the record, quoted the same way as `airports.dat`, for example
`"BOS","General Edward Lawrence Logan International Airport","Boston","United States","KBOS",42.36429977,-71.00520325`
(numbers as in `NUM` below). The codes are in preorder as the
project asks, and the file alone rebuilds the tree, including airports that are not in
`airports.dat` (such as `QQQ`). The alternative, saving only the codes, would need the records
from `airports.dat` at load time and could not restore `QQQ`.

The tree gives the records in preorder (`PREORDER`) and rebuilds itself from such a list
(`BUILD-FROM-PREORDER`); reading and writing the file is done next to `LOAD-AIRPORTS`, which
already has the CSV splitter.

### PREORDER(): main steps

```
PREORDER()                                    // explicit stack; the records in preorder
1. out ← empty list
2. if root = NULL then return out
3. S ← empty stack; push root
4. while S is not empty do
5.     node ← pop S; append node.data to out            // visit the node first
6.     if node.right ≠ NULL then push node.right      // right first, so left comes out first
7.     if node.left ≠ NULL then push node.left
8. return out
```

### SAVE(path): main steps

```
SAVE(path)
1. open path for writing; if it fails then report the error; return
2. list ← PREORDER()
3. for i ← 0 to size(list) − 1 do
4.     a ← list[i]
5.     write QUOTE(a.iata), QUOTE(a.name), QUOTE(a.city), QUOTE(a.country), QUOTE(a.icao),
          NUM(a.latitude), NUM(a.longitude) as one line
6. report size(list) lines written

QUOTE(s) = '"' + s with every " replaced by "" + '"'
NUM(x)   = x written with 15, 16 or 17 significant digits: the fewest that read back as exactly x
```

`NUM` keeps the file readable (`42.36429977`, not `42.364299770000003`) and the reloaded
numbers identical to the saved ones.

### BUILD-FROM-PREORDER(list, idx, lo, hi): main steps (method B3)

```
BUILD-FROM-PREORDER(list, idx, lo, hi)        // idx is shared by all calls (by reference)
1. if idx = size(list) then return NULL
2. key ← list[idx].iata
3. if (lo ≠ NONE and key ≤ lo) or (hi ≠ NONE and key ≥ hi) then return NULL   // belongs higher up
4. node ← new node(list[idx]); idx ← idx + 1
5. node.left ← BUILD-FROM-PREORDER(list, idx, lo, key)
6. node.right ← BUILD-FROM-PREORDER(list, idx, key, hi)
7. return node
```

Each record is taken exactly once, so it costs O(n).

### LOAD-TREE(path): main steps

```
LOAD-TREE(path)
1. open path; if it fails then report the error; return
2. list ← empty vector
3. for each line of the file (line number j) do
4.     f ← SPLIT-CSV-LINE(line)
5.     if size(f) ≠ 7 or length(f[0]) ≠ 3 or f[5], f[6] are not coordinates then
          report "bad line j"; return                               // old tree is kept
6.     append Airport(f[1], f[2], f[3], f[0], f[4], number(f[5]), number(f[6])) to list
7. idx ← 0; newRoot ← BUILD-FROM-PREORDER(list, idx, NONE, NONE)
8. if idx ≠ size(list) then free newRoot; report "not the preorder of a BST"; return
9. delete the old tree; root ← newRoot; count ← size(list)
10. VALIDATE(); print height and average depth
```

Step 8 catches a file that is out of order or has a duplicate: the build stops early. B3
takes every record exactly when the list is the preorder of some BST, and then the rebuilt
tree has exactly that preorder.

### DETECT-FILE(path): main steps

`load` reads both kinds of file. They differ in the number of fields per line.

```
DETECT-FILE(path)
1. if path cannot be opened then return CANNOT-OPEN
2. if the file has no line then return SAVED-TREE           // an empty tree was saved
3. n ← size(SPLIT-CSV-LINE(first line))
4. if n = 14 then return AIRPORTS-DAT                       // LOAD-AIRPORTS + BUILD-FROM-ARRAY
5. if n = 7 then return SAVED-TREE                          // LOAD-TREE
6. return UNKNOWN                                           // report; the tree is unchanged
```

Check (Task 4(b)): `SAVE` then `LOAD-TREE`, and compare the height and the full
`LEVEL-ORDER` list before and after; both must be equal.

---

## 9. Range query

Lists every airport with FROM ≤ code ≤ TO (both ends included; they do not have to exist in
the tree). A subtree is entered only if it can hold a code in the range.

### RANGE(node, from, to): main steps

```
RANGE(node, from, to)                         // appends matches to out in code order
1. if node = NULL then return
2. visited ← visited + 1
3. if from < node.key then RANGE(node.left, from, to)        // left subtree may hold codes ≥ from
4. if from ≤ node.key and node.key ≤ to then append node.data to out
5. if node.key < to then RANGE(node.right, from, to)         // right subtree may hold codes ≤ to
Initial call: out ← empty; visited ← 0; RANGE(root, FROM, TO)
If FROM > TO: report "FROM must not be after TO" and do nothing.
```

Pruning: if `node.key ≤ from`, every key in the left subtree is smaller than `from`, so step 3
skips it; the mirror case for the right subtree in step 5.

Output: `113 airports, 127 nodes visited` for `range BOS BWI` (sample session), followed by
one line per airport in code order.

The visited count depends on the exact pruning tests. With `from ≤ node.key` in step 3 the
same query still returns 113 airports but visits 128 nodes: at BOS itself it also enters the
left subtree, which can only hold codes below BOS. Steps 3 and 5 use strict `<`, so a subtree
is entered only if it can hold a code in the range.

Cost O(h + k): every visited node outside [FROM, TO] lies on the search path of FROM or of
TO, so at most |path(FROM)| + |path(TO)| visited nodes are not results. On the sorted chain
the same query visits 717 nodes: every code from AAA up to BWI is on the way.

---

## 10. Balanced build and rebalance

### BUILD-BALANCED(a, lo, hi): main steps (method B2, a is sorted by code)

```
BUILD-BALANCED(a, lo, hi)
1. if lo > hi then return NULL                       // empty range
2. mid ← ⌊(lo + hi) / 2⌋
3. node ← new node(a[mid])                           // middle record is the root
4. node.left ← BUILD-BALANCED(a, lo, mid − 1)
5. node.right ← BUILD-BALANCED(a, mid + 1, hi)
6. return node
Initial call: root ← BUILD-BALANCED(a, 0, n − 1); count ← n
```

Height ⌊log₂ n⌋ = 12 for n = 6,072. Each call splits its range into two halves whose sizes
differ by at most one, so every level except the last is full. The sum of the depths is
then the smallest possible for n nodes: Σ ⌊log₂ i⌋ for i = 1 … n (the depths in a complete
tree numbered 1 … n level by level).

The list must be strictly increasing, or the result is not a BST. The public operation
checks that first:

```
BUILD-BALANCED-TREE(a, n)
1. if IS-SORTED(a, n) is FALSE then report it; return FALSE      // the old tree is kept
2. delete every node; root ← BUILD-BALANCED(a, 0, n − 1); count ← n
3. return TRUE
```

### REBALANCE(): main steps (lecture Section 3.5, Application 1)

```
REBALANCE()
1. a ← INORDER()                                     // already sorted (P1)
2. BUILD-BALANCED-TREE(a, size(a))
3. print height and average depth                    // sample session line 14
```

Expected after `remove BOS` (n = 6,071): height 12, average depth 10.7. `INORDER` uses an
explicit stack, so `rebalance` also works on the 6,071-level chain.

---

## 11. Shape experiment

The same 6,072 records, built four ways; the same 10,000 search keys for every tree.

### SHAPE-EXPERIMENT(records, n): main steps

```
SHAPE-EXPERIMENT(records, n, Q = 10,000, R = 7)
 1. sorted ← copy of records, sorted by code
 2. queries ← Q codes: queries[i] ← records[NEXT(rng) mod n].iata   // rng seeded with 2560
 3. T1 ← BUILD-FROM-ARRAY(records, n)                    // file order        (build timed)
3b. search T1 again and again, untimed, for at least 200 ms   // let the processor speed up
 4. T2 ← BUILD-FROM-ARRAY(sorted, n)                     // sorted order: a chain
 5. T3 ← BUILD-BALANCED-TREE(sorted, n)                  // middle rule
5b. T4 ← BUILD-FROM-ARRAY(SHUFFLE(records, 2560), n)     // random order (bonus row)
 6. for each tree T in T1, T2, T3, T4 do
 7.     (h, avg) ← HEIGHT-AND-DEPTH(T); VALIDATE(T)
 8.     run the Q searches once without timing           // warm-up: caches, first touches
 9.     for r ← 0 to R − 1 do
10.         start ← clock
11.         for i ← 0 to Q − 1 do
12.             if COUNT-FIND(T, queries[i], c) then found ← found + 1
13.             comparisons ← comparisons + c
14.         time[r] ← clock − start
15.     report h, avg, the median of time[0 … R − 1], comparisons / Q, found
```

- `COUNT-FIND` is `FIND` without building the path list: it only counts comparisons, so the
  timing measures the search itself. The comparison and found totals are printed, which
  keeps the compiler from removing the loop.
- Building the trees, sorting and drawing the queries are not inside the search timing.
  Times come from a `-O2` build with `std::chrono::steady_clock`; the median of R runs is
  less disturbed by other programs than one run.
- Comparisons per search is a machine-independent check: when the queries are drawn
  uniformly from all keys, it should be close to average depth + 1.

Expected heights: file order 177, sorted 6,071, balanced 12, random about 25–29.

### SHUFFLE(records, seed) and the seed sweep (Task 6 bonus)

`rng` is `std::mt19937`, whose output sequence is fixed by the C++ standard, and `NEXT(rng)`
is its next 32-bit number. The library's `std::shuffle` and `uniform_int_distribution` are
not used, because their algorithms differ between standard libraries; with plain `mod` the
same seed gives the same order and the same queries with every compiler. (The bias of
`mod n` is at most n / 2³² ≈ 1.4 · 10⁻⁶.)

```
SHUFFLE(a, seed)                              // Fisher–Yates, front to back
1. rng ← mt19937(seed); n ← size(a)
2. for i ← 0 to n − 2 do
3.     j ← i + (NEXT(rng) mod (n − i))        // a random index in i … n − 1
4.     swap a[i] and a[j]
5. return a

SEED-SWEEP(records, n, S = 100)               // how much the height depends on the seed
1. for s ← 1 to S do
2.     T ← BUILD-FROM-ARRAY(SHUFFLE(records, s), n)
3.     record HEIGHT-AND-DEPTH(T)
4. report min / median / max height and the mean average depth
```

For a random insertion order the expected average depth is 2(1 + 1/n)·H_n − 4, where H_n is the
harmonic number; for n = 6,072 that is about 14.6.

---

## 12. Command loop

```
MAIN
1. tree ← empty AirportBST; records ← empty
2. while a command line can be read do
3.     split it into words; convert codes to upper case
4.     dispatch on the first word (unknown command → print help)
```

| Command | Steps |
|---|---|
| `load FILE` | first line has 14 fields → `LOAD-AIRPORTS` + `BUILD-FROM-ARRAY` + `HEIGHT-AND-DEPTH`; otherwise → `LOAD-TREE` |
| `find CODE` | `FIND`, print path, comparisons and record |
| `min`, `max` | `MINIMUM`, `MAXIMUM` |
| `remove CODE` | `REMOVE`, print the case, then `VALIDATE` |
| `add CODE NAME, CITY, COUNTRY[, ICAO, LAT, LON]` | `ADD` below; then `VALIDATE` (from Task 4(c)) |
| `list` | `INORDER`, print, `IS-SORTED` |
| `levels [k] [CODE]` | `TOP-LEVELS(k, start)`, k from 1 to 6, default 3; start = root or CODE |
| `validate` | `VALIDATE` |
| `save FILE` | `SAVE` |
| `range FROM TO` | `RANGE` |
| `rebalance` | `REBALANCE` |
| `stats` | size, `HEIGHT-AND-DEPTH` |
| `quit` | stop |

`load` serves both file types because the sample session loads `airports.dat` with it and
Task 4(b) reloads a saved file with it; `DETECT-FILE` tells them apart. A failed `load`
keeps the old tree.

### ADD(line): main steps (Task 3(c))

The fields after the code are in the order `find` prints them: name, city, country. They are
split with `SPLIT-CSV-LINE`, so a name that contains a comma is written in quotes.

```
ADD(line)                                     // add CODE NAME, CITY, COUNTRY[, ICAO, LAT, LON]
1. code ← the first word after "add", in upper case; rest ← the text after the code
2. if code is not 3 characters, each a letter or a digit then report "bad code"; return
3. f ← SPLIT-CSV-LINE(rest); remove the spaces around every field
4. if size(f) is not 3 or 6, or f[0], f[1] or f[2] is empty then print the usage; return
5. icao ← ""; lat ← 0; lon ← 0                          // defaults for a made-up airport
6. if size(f) = 6 then
7.     icao ← f[3]
8.     if f[4], f[5] are not coordinates then report "bad coordinates"; return   // as in LOAD-AIRPORTS
9.     lat ← number(f[4]); lon ← number(f[5])
10. if INSERT(Airport(f[0], f[1], f[2], code, icao, lat, lon)) then
11.     report "Added", the record and its depth (comparisons of FIND − 1)
12. else report "already in the tree" (I4)             // the tree is unchanged
```

A new code is always inserted as a leaf (I1–I3), so removing it straight away is case D1 and
gives back exactly the tree from before the add.

Input rules: command words are not case sensitive; codes are converted to upper case; empty
lines and lines starting with `#` are ignored (comments in a script). When the commands
come from a file or pipe, each one is printed after the `> ` prompt, so the output reads like
the sample session; typed commands are not printed twice.

---

## 13. Cost summary

h is the height: 177 for the file order, 6,071 for the sorted order, 12 balanced.

| Operation | Time | Extra space |
|---|---|---|
| `INSERT`, `FIND`, `MINIMUM`, `MAXIMUM` | O(h) | O(1) |
| `REMOVE` | O(h) (search + walk to the successor) | O(1) |
| `BUILD-FROM-ARRAY` | Σ depths: O(n log n) random order, O(n²) sorted | O(n) nodes |
| `HEIGHT-AND-DEPTH`, `LEVEL-ORDER` | O(n) | O(width) queue |
| `INORDER`, `SAVE` | O(n) | O(h) stack |
| `IS-BST` / `VALIDATE` | O(n) | O(h) recursion |
| `BUILD-FROM-PREORDER` / `LOAD-TREE` | O(n) | O(h) recursion |
| `RANGE` | O(h + k), k = number of results | O(h) recursion |
| `BUILD-BALANCED` / `REBALANCE` | O(n) | O(log n) recursion + O(n) array |
| `LOAD-AIRPORTS` | O(total characters) | O(n) records |
