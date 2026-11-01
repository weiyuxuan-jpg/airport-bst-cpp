# Screen recording: how it is made

The project asks for a short screen recording or GIF of a session that shows `find`,
`range`, the removal of a node with two children, and `validate`. These steps produce a
recording of about one minute.

## The recording in this folder

[`demo.mov`](demo.mov) (48 seconds, 2940 × 1912, H.264, no sound) was recorded on
2026-11-01 on macOS 26.6 with the system's own `screencapture -v -V 48`, with the terminal
filling the screen.

The commands in it were **not typed by hand**. A shell script printed the line
`$ ./airports data/airports.dat`, then started the program and sent it the commands of
`tests/sessions/demo.txt` (without the comment lines), one every 5 seconds. Input that
does not come from a keyboard is echoed by the program after its `>` prompt, which is how
the commands appear on screen. Everything after the first line is the program's own output,
the same as in the table below.

## Before recording

1. Open a terminal in the project folder and make the window large enough for about
   30 lines of 120 characters. Use a large font so the video is readable.
2. Build: `g++ -std=c++17 -O2 src/*.cpp -o airports`. On a Mac with the SDK problem
   described in the README, prefix the command with `SDKROOT=…`.
3. Run `clear`.

## Recording on macOS

- Press **Shift-⌘-5**, choose *Record Selected Portion*, select the terminal window, then
  *Record*. To stop, click the stop button in the menu bar. The video is saved as a `.mov`
  file on the Desktop unless *Options → Save to* names another folder.
- Save it, or move it afterwards, as **`docs/demo.mov`** in this project folder.
- A GIF needs a converter, for example `ffmpeg` (not installed on this machine).

## What to type

Type the commands by hand, pausing about two seconds after each one so the viewer can read
the output. They are the same as in `tests/sessions/demo.txt`.

| # | Type | What the viewer should see |
|---|---|---|
| 1 | `./airports data/airports.dat` | `Loaded 6072 airports (1626 lines skipped: no IATA code). Duplicates: 0` and `Height: 177   Average depth: 18.3` |
| 2 | `find BOS` | the path `GKA -> AEY -> … -> BOU -> BOS   (comparisons: 12)` and the Boston airport |
| 3 | `find XYZ` | a path that ends in `(NULL)   not found (comparisons: 14)` |
| 4 | `range BOS BOZ` | `5 airports, 18 nodes visited` and the five airports BOS, BOU, BOW, BOX, BOY. (`range BOS BWI`, the project's example, prints 113 lines and scrolls its first line off the screen.) |
| 5 | `levels` | `Level 0: GKA` / `Level 1: AEY MAG` / `Level 2: AAE EGS HGU POM` |
| 6 | `remove AEY` | `Deleted AEY (case D3: two children, successor AFA)   validate: OK`: AEY has two children, so its successor AFA is copied into its place |
| 7 | `levels` | `Level 1: AFA MAG`: AFA now stands where AEY was |
| 8 | `validate` | `validate: OK (6071 nodes, each inside its (lo, hi) range)` |
| 9 | `quit` | back at the shell prompt |

The expected output is exactly what `./airports data/airports.dat < tests/sessions/demo.txt`
prints. Running that once before recording shows that everything is in place.

## After recording

- Trim the video if needed (QuickTime Player: *Edit → Trim*).
- Keep the file small; GitHub accepts files up to 100 MB, and a one-minute terminal
  recording is usually a few MB.
- Put it in the repository as `docs/demo.mov` (or `docs/demo.gif`) and link it from the
  README; or upload it in GitHub's web editor, which embeds the video in the page.
