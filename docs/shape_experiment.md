# Shape experiment (project Task 6)

The same 6,072 airport records (all `airports.dat` lines with an IATA code) are built into a
BST in four ways, and the same 10,000 random searches are timed on each tree.

Reproduce with:

```
g++ -std=c++17 -O2 src/*.cpp -o airports
printf 'experiment\nquit\n' | ./airports
```

The facts used in the explanation are measured and checked by `tests/test_file_order.cpp`.

## Method

- **Trees.**
  - File order: insert the records in the order of the file (`BUILD-FROM-ARRAY`).
  - Sorted order: insert them sorted by code.
  - Balanced: build directly from the sorted list with the middle rule (`BUILD-BALANCED`,
    lecture method B2).
  - Random shuffle: insert them in a random order (Fisher–Yates, seed 2560).

  Every tree passes `validate`.
- **Queries.** 10,000 codes drawn uniformly at random from the 6,072 codes (seed 2560). Every
  tree gets the same list, and every query is found.
- **Random numbers.** `std::mt19937` with plain `mod`. The generator's output is fixed by the
  C++ standard; the library's `std::shuffle` and `uniform_int_distribution` are not used,
  because their algorithms differ between standard libraries. So the same seed gives the
  same shuffle and the same queries with every compiler.
- **Timing.** `std::chrono::steady_clock` around the 10,000 searches only. The searches use
  `countFind`, which is `find` without building the path. Before any timing there are at
  least 200 ms of untimed searches; then each tree gets one untimed pass and 7 timed
  passes, and the table shows the median. Building the trees, sorting, shuffling and drawing
  the queries are not inside the search time.
- **Machine.** Apple M5 (10 cores), macOS 26.6, Apple clang 21.0.0 with libc++,
  `g++ -std=c++17 -O2`.
- **Machine-independent measure.** The mean number of comparisons per search. With queries
  drawn uniformly from all keys it should be close to average depth + 1.

## Results (2026-10-21)

| Build order | Height | Average depth | Time for 10,000 searches | Comparisons per search | Your explanation |
|---|---:|---:|---:|---:|---|
| File order | **177** | 18.32 | 1.01 – 1.17 ms | 19.3 | The file is sorted by airport ID, and the IDs were given country by country in ICAO order. For the 181 Canadian airports (lines 21–205) the IATA code is the ICAO code without the "C", so their codes arrive sorted. The run YDA … ZFM (162 codes) is inserted as a chain of right children; 155 of them lie on the deepest path. The rest of the file is close to random order. |
| Sorted order | **6,071** | 3,035.5 | 109 – 130 ms | 3,007.7 | Every new key is larger than all keys before it, so every insert goes right: a chain of n − 1 edges, the worst case (lecture P4). A search walks half the chain on average. |
| Balanced (middle rule) | **12** | 10.65 | 0.61 – 0.65 ms | 11.7 | The middle element becomes the root and both halves are built the same way, so every level is full except the last: height ⌊log₂ 6072⌋ = 12 and the smallest possible total depth. |
| Random shuffle (bonus) | **26** | 14.2 | 0.86 – 0.91 ms | 15.2 | A random order has no long increasing runs (the longest is 7). Over 100 seeds the height is 24 – 34 (median 28) and the mean average depth is 14.57, close to the theory value 14.58 for random insertion. |

- **Time** is the median of 7 timed runs; the table gives the range of that median over three
  separate runs of the program (listed below).
- **Height, average depth and comparisons** are the same in every run.
- **Expected by the project**: heights 177, 6,071 and 12, and about 25–29 for a random
  shuffle. The first three match exactly. The shuffle with seed 2560 gives 26, and seeds
  1–100 give 24–34 with median 28.

### The three runs

Search time over the 7 timed runs, min / median / max, in ms:

| Run | File order | Sorted order | Balanced | Random shuffle |
|---|---|---|---|---|
| 1 | 1.013 / 1.015 / 1.072 | 90.011 / 129.601 / 131.952 | 0.604 / 0.607 / 0.663 | 0.857 / 0.863 / 0.905 |
| 2 | 1.004 / 1.010 / 1.050 | 121.129 / 121.410 / 122.255 | 0.598 / 0.605 / 0.618 | 0.850 / 0.866 / 0.932 |
| 3 | 1.148 / 1.165 / 1.193 | 99.343 / 108.897 / 116.982 | 0.650 / 0.654 / 0.694 | 0.884 / 0.912 / 0.982 |

Time to build each tree once: file order 0.79 – 0.85 ms, sorted order 65 – 67 ms,
balanced 0.21 – 0.22 ms, random shuffle 0.71 – 0.73 ms.

## Why the file order gives height 177

**1. How the file is ordered.** The lines of `airports.dat` are in increasing order of
airport ID, and the early IDs were given country by country:
- the first lines are Papua New Guinea (lines 1–6), Greenland and Iceland;
- lines 21–205 (IDs 21–206) are Canada.

Inside the Canada block the airports are listed in order of their ICAO code (CYAM, CYAV, …,
CZFM), with one entry out of place: CYZS on line 41 comes before CYDA on line 42. For all 181
of these airports the IATA code is the ICAO code without the leading "C" (CYDA → YDA). So
the IATA codes of lines 21–205 come in increasing order, apart from that one break.

**2. What a sorted stretch does to a BST.** When codes arrive in increasing order and all of
them fall into the same gap of the tree, each one is larger than the one before, so each
becomes the right child of the one before. That is the chain of the lecture's Figure 5, built
here from 162 codes instead of 7.

**3. The deepest path.** The deepest node is YZR at depth 177. Its 178-node path is built
from exactly these lines:

| Part of the path | Nodes | From the file |
|---|---:|---|
| GKA, MAG, POM, WWK | 4 | Papua New Guinea, lines 1–6: the first codes of the file |
| YAM, YAY, …, YCY, then YZS | 19 | Canada, lines 21–41: increasing, each one a right child |
| YDA, YDB, …, YZR | 155 | Canada, lines 42–205: the longest increasing run of the file (162 codes, YDA … ZFM) |

YDA is smaller than YZS but larger than YCY, so the long run falls into the single gap
between YCY and YZS. YDA becomes the left child of YZS, and all 154 steps after it go to the
right. 4 + 19 + 155 = 178 nodes, so the height is 177.

**4. Why nothing later repairs it.** An insert only adds a leaf; it never moves an existing
node (I1–I3). Once the chain is built, its nodes keep their depths whatever is inserted
afterwards. Inserting later codes into the gaps of the chain only hangs short subtrees on
it. The rest of the file is much closer to a random order: 57.8% of neighbouring codes
increase, against 50.5% after a shuffle. Its other increasing runs are far shorter than 162:
the next longest has 35 codes (United States, lines 4437–4471), then 30, 29, 26 and 21.

**5. How much it costs.** The 162 codes of the long run lie at an average depth of 96.8.
On their own they add 2.58 to the average depth of the whole tree (18.32). The other 5,910
nodes average 16.17. That is still worse than a random order (14.57); the rest of the file
also has more increasing neighbours than a random order, and five more runs of 21–35 codes. A search for a Canadian code such as YZR needs up to 178
comparisons; in the balanced tree no search needs more than 13.

**6. Why a random order is so much better.** Shuffling breaks the runs:
- after the shuffle the longest increasing run is 7 codes, against 162 in file order, and the
  file order has 2,563 ascending runs against 3,007 after the shuffle;
- the height of a randomly built BST grows like O(log n): for n = 6,072, 100 shuffles gave
  24 – 34;
- the expected average depth is 2(1 + 1/n)·H_n − 4 ≈ 14.58 (H_n is the harmonic number), and
  the measured mean was 14.57.

**Connection to O(h).** Every operation follows one path from the root, so its cost is set
by the height:

| Build order | Height | Comparisons per search |
|---|---:|---:|
| Balanced | 12 | 11.7 |
| Random shuffle | 26 | 15.2 |
| File order | 177 | 19.3 |
| Sorted order | 6,071 | 3,007.7 |

The search times follow the comparisons, roughly. On uniform queries the file-order tree is
only 1.5–1.9 times slower than the balanced one, because few airports lie on its deep chain.
For queries on the Canadian codes it is far worse.

## Other observations

- **Comparisons follow the depth.** 19.3 ≈ 18.32 + 1, 3,007.7 ≈ 3,035.5 + 1, 11.7 ≈ 10.65 + 1
  and 15.2 ≈ 14.2 + 1; the small differences come from the random sample of 10,000. The C++
  test also checks that the experiment's count equals the length of `find`'s search path for
  the same queries.
- **Time per comparison.** It is about 5.2–6.0 ns for the file-order, balanced and random
  trees, and about 3.6–4.3 ns for the sorted chain. A likely reason, not measured here, is
  that the chain's nodes were allocated one after another and are visited in that order.
- **Sorted order is also slow to build.** The i-th insert walks i − 1 nodes:
  n(n − 1)/2 = 18,431,556 comparisons in total, against the sum of the depths, about 111,000,
  for file order.
- **Timing noise.** Without the 200 ms warm-up, the first tree measured (file order) took
  0.99–1.99 ms over six runs instead of about 1.0 ms. The sorted-order median still moves
  between runs (75–137 ms over all runs made during development). Each of its timed runs
  lasts about a tenth of a second and all of them about a second, so the operating system
  can move the program between the processor's fast and slow cores. The comparison counts
  do not move.
