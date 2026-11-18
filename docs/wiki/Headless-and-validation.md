# Headless simulation, tests and reproduction

[MazeRunner V5.4.3a wiki](Home.md)

## Build and use headless simulation

A headless executable runs the same maze, monster, director, and auto-player logic without a window. It needs no GLFW/GLEW headers or graphics linkage because `MAZERUNNER_HEADLESS` removes those sections. Use it for reproducible experiments, automated tests, or machines without a display. `--simulate` enables auto-solving at time zero, which also releases the monsters, so play starts immediately.

### Ubuntu/Linux

From the package root after creating `build`:

```bash
g++ src/mazerunnerV5.4.3a.cpp -o build/maze-headless -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -DMAZERUNNER_HEADLESS -pthread
./build/maze-headless --simulate 33 33 5 42 60000 --difficulty medium --quiet
```

### Apple Silicon macOS

```bash
clang++ src/mazerunnerV5.4.3a.cpp -o build/maze-headless -std=c++17 -O2 -DNDEBUG -arch arm64 -Wall -Wextra -Wpedantic -DMAZERUNNER_HEADLESS -pthread
./build/maze-headless --simulate 33 33 5 42 60000 --difficulty medium --quiet
```

### Windows CMD, with the UCRT64 PATH already selected

```bat
g++ src\mazerunnerV5.4.3a.cpp -o build\maze-headless.exe -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -DMAZERUNNER_HEADLESS -pthread
build\maze-headless.exe --simulate 33 33 5 42 60000 --difficulty medium --quiet
```

### Argument meanings and result interpretation

The positional form is:

```text
--simulate ROWS COLS MONSTERS SEED [LIMIT_MS] [--difficulty low|medium|high] [--verbose] [--quiet]
```

The first two values are **rows then columns**, whereas the setup summary displays **width × height**. In a square example they happen to match. `MONSTERS` is 0–10. `SEED` is an unsigned 32-bit integer from 0 to 4,294,967,295. The optional duration is a simulated-time ceiling in milliseconds, defaulting to 120,000 and bounded by the parser at 3,600,000. `--simulate` must be the first argument. Dimensions are validated and normalized by the Maze constructor; use 5–501 in supported runs.

The final line identifies version, difficulty, seed, dimensions, monster count, outcome, simulation time, automatic steps, and host compute time. `ESCAPED` means escape, `CAUGHT` means collision, and `RUNNING` means the simulation limit was reached before either terminal outcome. All three are valid runs and return process exit code zero. Input/build/runtime failures return a nonzero status, generally one from the program's top-level exception handler. A benchmark must parse the outcome rather than counting exit code zero as a win.

Run without `--quiet` to see “Local footstep area”, “Heard by”, “Director: REPORT”, “Investigating by radio”, and precision transitions with the current radio area, retasked monsters and released interceptors. Direct High sightings and hunt/search state transitions also appear when they occur. Add `--verbose` for individual monster moves.

`simulation_ms` is game time; `compute_ms` measures host processing time for the simulation advance, excluding generation. It varies with load, compiler, and machine. Exclude `compute_ms` when comparing deterministic output. The seed controls pseudorandom choices, not difficulty or a guaranteed maze shape across different C++ library implementations. `std::mt19937` is specified, but higher-level distributions/shuffle can differ between standard libraries. Expect repeatability within the same source/settings/toolchain; validate invariants instead of requiring byte-identical cross-platform trajectories.

## Automated test reproduction on Linux

### Step 1 — verify the committed package

Before running anything that overwrites committed results:

```bash
python3 scripts/verify_package.py
```

This verifies the manifest's payload hashes, the source identity, the exact three-document set, one source-matching annotation per physical line, local links, and the eighteen screenshots. It is an integrity/documentation check, not an additional gameplay simulation. It prints a success report or exits nonzero with a concrete mismatch. A modified source or regenerated log will intentionally differ from the committed manifest; after an intended change, `python3 scripts/audit_documentation.py` regenerates the coverage/audit records, `manifest.json`, and `SHA256SUMS`.

If you want to retain the committed evidence, use a second checkout for reruns. The validation runner writes fresh files under `artifacts/results` and `artifacts/screenshots` and overwrites matching filenames. It does not edit production source. After rerunning, do not expect the original package checksums to match new host timings and captures.

### Step 2 — install test prerequisites

For Ubuntu with GCC already installed:

```bash
sudo apt install python3 cmake ninja-build libegl1 libgl1 libegl-mesa0 libgl1-mesa-dri
```

The portable core tests need only Python and a C++17 compiler. The graphics harness additionally uses Linux EGL/GL shared objects and Mesa's surfaceless path. It declares its small EGL/GL interface through the included test adapters, so it does not need the native GLFW development package to run offscreen. This deliberately tests another boundary from a real desktop-window build.

### Step 3 — run the full packaged validation

```bash
python3 scripts/run_validation.py --all
```

The script locates the package root from its own path, creates output directories, invokes the compiler directly, and stops on a failed command/assertion. It records the precise compiler/test commands, return codes, and elapsed host durations in `artifacts/results/validation-summary.json`. It uses Python's standard library only. The recorded run finished all checks successfully.

For a machine without the Linux offscreen graphics path, run only the portable portion:

```bash
python3 scripts/run_validation.py --core
```

To run only rendering/application checks after changing graphics:

```bash
python3 scripts/run_validation.py --graphics
```

For the CMake/CTest record and the escape-rate benchmark:

```bash
python3 scripts/run_cmake_validation.py
python3 scripts/run_calibration.py
```

The calibration runner builds a fresh optimized headless binary. Its defaults run 2,400 balance games, 540 ten-monster preset games, 400 default-setup games and 1,400 fresh-seed holdouts. `--seeds`, `--preset-seeds`, and `--holdout-seeds` control the row counts for a local experiment. `--jobs` limits simultaneous processes. The raw CSV is ordered by cohort, map, difficulty and seed even with multiple workers. Time ceilings and partial runs appear explicitly in the data. [Raw CSV](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/calibration-runs.csv) · [Aggregate JSON](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/calibration.json)

An explicit compiler can be selected with `--compiler g++`. If the local compiler/runtime cannot support the sanitizer configuration, `--skip-sanitizers` records that check as skipped. A skipped sanitizer is not equivalent to the bundled successful sanitizer result. The runner and its environment-report wording are intended for Linux; use the manual portable commands in the next section on macOS/Windows.

The graphics harness requests a real offscreen desktop-OpenGL core context via Mesa's surfaceless EGL platform and uses actual driver calls for shaders, buffers, draws, and readback. It is not a mock that accepts all drawing commands. Conversely, its GL/GLFW support headers are **test-only** adapters. Never add `-Itests/support/...` to the normal graphical game build: doing so would replace real dependency headers and skip the very integration you need to validate.

### What the runner executes

| Stage | Coverage and output |
|---|---|
| Headless compilation/help | C++17, `-O2`, `-Wall -Wextra -Wpedantic -Werror -pthread`; real CLI help |
| Full portable regressions | `ui_tests.cpp` includes the previous suites and V5.4.3a director, weighted-overlay and hearing regressions |
| Director traces | First report on 63×63 and a complete Medium relief/recovery cycle on the default rectangle |
| Sanitized regressions | Same suite with `-O1 -g -fsanitize=address,undefined` |
| Preset/profile smoke matrix | Sizes 33/63/101 × Low/Medium/High; ten monsters, seed 42, 1,000 ms; each case run twice |
| Graphical syntax | Full source syntax using declaration-only test headers; no native GLFW/GLEW link claim |
| Real-GL renderer | Shader compile/link, wall/actor visibility and palette, aspect, zoom, minimized targets, all ten variants, 501×501 mesh, screenshots |
| Application integration | Production `GameApplication`/callbacks with injected window data and real GL; loading, settings transfer, focus/pause, replay, results, failures, cleanup |

The sanitizer run sets `ASAN_OPTIONS=detect_leaks=0` and `UBSAN_OPTIONS=halt_on_error=1`. Address and undefined-behavior checks passed; **leak detection was disabled**, so this is not a leak-audit result. The sanitizer pass covers the portable suite, not the EGL/driver process or a native window session.

### Test-source map

| Test file | What to inspect or extend |
|---|---|
| [feedback_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/feedback_tests.cpp) | Start grace, hearing range, roam/hunt/search transitions, approximate hints, waits and invalid movement, full overlays, route diversity (with an independent A* oracle), core regressions, terminal logs |
| [director_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/director_tests.cpp) | Bounded waits, committed retreats, alternate-branch backtracking, hint privacy/expansion, relief recovery, no relief for an idle player, event partitioning |
| [difficulty_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/difficulty_tests.cpp) | Parsing/profile values, frozen-until-start monsters, exact movement/scent/report timings, all director profiles, exhaustive small-graph diversity checks, per-profile invariants |
| [v542_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/v542_tests.cpp) | Retained start grace, searching, interceptors, corridor sight, timing-aware blocking, generation fallback, terrain guard and searching-eye tests |
| [v543a_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/v543a_tests.cpp) | Radio delivery beyond hearing, quantized privacy and cooldowns, scent expiry and fallback, relief cycles, guard release/cap, weighted path detour and hearing through corridors |
| [visual_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/visual_tests.cpp) | Wall/sprite geometry, camera projection, silhouette uniqueness, inclusion of gameplay/profile regressions |
| [ui_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/ui_tests.cpp) | 33/63/101 presets, dimensions/count validation, pointer and keyboard focus, DPI transform, transitions; top-level portable runner |
| [render_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/render_tests.cpp) | Real offscreen draw/readback, screenshot fixtures, actual shaders, sprite visibility, large-map mesh |
| [app_tests.cpp](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/app_tests.cpp) | Injected production callbacks, async loading, pauses, Esc never quitting, hold-to-move timing and release, replay seeding, real-GL UI/world composition, error recovery |
| [egl_test_context.h](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/tests/egl_test_context.h) | Offscreen application-test context creation and destruction |

Tests include the production `.cpp` with macros that remove its normal `main` or selected window-startup sections. Compile the specified test entry point **alone**; do not also append the production `.cpp` to that test compile command, or duplicate definitions will result. Helper test files are included transitively and are not all standalone executables.

## Manual portable tests on macOS and Windows

These commands compile the same core suite without the Linux-specific EGL harness. They are provided for local execution; there are no bundled macOS/Windows result logs claiming they passed.

### macOS, from the package root

```bash
mkdir -p build
clang++ tests/ui_tests.cpp -o build/regressions -std=c++17 -O2 -arch arm64 -Wall -Wextra -Wpedantic -pthread
./build/regressions
```

If supported by the installed Apple toolchain, a separate sanitizer run is:

```bash
clang++ tests/ui_tests.cpp -o build/regressions-asan -std=c++17 -O1 -g -arch arm64 -fsanitize=address,undefined -pthread
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ./build/regressions-asan
```

Record the compiler version and command output with the result. This tests the headless simulation/UI model and geometry helpers, not a Cocoa window or Retina rendering. Perform the native desktop checklist as well.

### Windows CMD, after selecting UCRT64 PATH

```bat
if not exist build mkdir build
g++ tests\ui_tests.cpp -o build\regressions.exe -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread
build\regressions.exe
```

Use the same compiler-runtime DLL search path as for the game. The Linux sanitizer and EGL instructions are not presented as supported native Windows steps. A Windows debugging/sanitizer setup requires validation of that specific toolchain's support.

## Recorded V5.4.3a results

| Evidence | Recorded result |
|---|---|
| [Direct runner](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/validation-summary.json) | All 32 recorded commands passed, including two director scenarios. Logs contain exact arguments, return codes and timings. |
| [Portable regressions](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/gameplay-and-ui.log) | Gameplay, settings, director, route, graphics geometry and UI passed. The printed 1,705,277 and 5,221,719 values are cumulative assertion counters from nested suites. |
| [ASan/UBSan](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/sanitizers.log) | Same portable suite passed with address and undefined-behavior instrumentation. Leak detection was disabled. |
| [Application](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/app.log) | Real GL drawing, injected GLFW events, stale key cancellation, time partitioning, async generation and error recovery passed. “Injected generation failure” is the intentional recovery fixture. |
| [Rendering](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/render.log) | OpenGL 4.5 core, Mesa llvmpipe. Real shader compilation, geometry, depth, aspect ratio, all ten appearances, zoom and maximum-size mesh passed. |
| [Ubuntu CTest](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/cmake/validation.json) | Release graphical and headless builds, 13/13 tests. The build and CTest logs accompany the record. |
| [Seed-by-seed games](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/calibration-runs.csv) | 4,740 rows with map, seed, profile, result, simulated time and move count. |
| [Documentation coverage](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/documentation-coverage.json) | All 3,261 lines of the shipped source quoted exactly and explained. |

### Reproduce the two director examples

From the root, first run the headless direct build or `python3 scripts/run_validation.py --core`. The runner places the CLI at `build/maze-headless`. With CMake use `build/cmake/bin/maze-headless` for the build directory in section 2.

```bash
./build/maze-headless --simulate 63 63 5 42 50 --difficulty medium
./build/maze-headless --simulate 21 31 5 7 32000 --difficulty medium
```

At 40 ms the first run reports rows 0–16 and columns 0–16. All five monsters investigate by radio while none has local evidence. In the second run relief starts at 4.200 s, retasks five monsters and releases one interceptor. The radio area grows from 17×17 to 21×25 and later 21×31. Normal precision returns at 17.600 s. The solver escapes at 31.400 s with 462 successful automatic moves. [First report](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/director-radio.log) · [Full relief trace](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/director-relief.log)

### Exact preset smoke outcomes

Every entry uses ten monsters and seed 42. Two invocations produced identical state and step counts through 1,000 simulated milliseconds after excluding host compute time. These short runs check reproducibility. The longer benchmark below measures escape frequency.

| Map | Difficulty | Outcome at 1,000 ms | Successful moves | Repeat identical |
|---|---|---|---:|---|
| 33×33 | Low | RUNNING | 9 | Yes |
| 33×33 | Medium | RUNNING | 24 | Yes |
| 33×33 | High | RUNNING | 4 | Yes |
| 63×63 | Low | RUNNING | 25 | Yes |
| 63×63 | Medium | RUNNING | 25 | Yes |
| 63×63 | High | RUNNING | 25 | Yes |
| 101×101 | Low | RUNNING | 25 | Yes |
| 101×101 | Medium | RUNNING | 25 | Yes |
| 101×101 | High | RUNNING | 25 | Yes |

### Escape rate and seed protocol

Five-monster calibration on 21×21 and 31×31 used seeds 1–400 per profile per size and a 60-second simulation ceiling. Escapes pooled to 751/800 Low, 536/800 Medium and 273/800 High. Ten-monster presets used 33×33, 63×63 and 101×101, each with seeds 1–60 per difficulty and a 120-second ceiling. The actual setup default is 21 rows × 31 columns, five monsters, Medium. It escaped in 260/400 games using seeds 401–800. Separate evaluations on seeds 1201–1400 give 1,400 further games across the two square maps and the default rectangle. Read the row counts, timeouts and Wilson intervals together. [Per-map tables](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/02-AI-Graphics-and-Redesign.md) · [Benchmark runner](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/scripts/run_calibration.py)
