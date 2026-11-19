# MazeRunner V5.4.3b

MazeRunner is a C++17 maze game with an in-game setup screen, pixel-art characters, raised gray walls on a black background, and a headless simulator for repeatable AI experiments. Play manually or enable the automatic solver while roaming monsters and an adaptive director respond to movement.

![MazeRunner setup screen](../artifacts/screenshots/setup.png)

The setup screen offers Low, Medium, and High difficulty, a configurable monster count, and three maze presets: **Small 33 × 33**, **Standard 63 × 63**, and **Large 101 × 101**. The game includes ten monster sprite variants, full-route visualization, and an automatic solver that can wait, take alternate routes, and backtrack. Player and monster state logs remain available in the terminal.

![MazeRunner game view](../artifacts/screenshots/game.png)

## What's new in V5.4.3b

One investigator follows the best clue available to it. Other informed monsters cover different sides of the reported sector. Eligible Cutters can intercept at a chokepoint. A searcher checks junctions and alcoves, briefly turns toward an unchecked arm, and remembers where it has looked.

The Director samples menace every 200 ms. Sustained pressure can send the investigator walking away. During this quiet period the pack still hears real steps and can make contact. The HUD shows *BACKSTAGE* and the menace value (`M`). Individual monsters can learn from repeated audible scent losses, sector returns, sustained headings and reversals. A new run clears this memory.

On seed 8 the lead withdraws at 8.600 seconds. It takes nine steps away before the solver reaches the exit at 13.040 seconds. [Encounter trace](../artifacts/results/director-relief.log)

![Backstage and menace on the HUD](../artifacts/screenshots/hud.png)

The [player and developer Wiki](https://github.com/LeEz1219/MazeRunner/wiki) includes a first-maze walkthrough, a guide to the Director HUD, and the five-monster calibration and holdout results.

## Build and play

On Ubuntu, install the compiler and graphical development dependencies, then run these commands from the repository root:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libglfw3-dev libglew-dev libgl1-mesa-dev libegl-dev pkg-config
cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --parallel
ctest --test-dir build/cmake --output-on-failure
./build/cmake/bin/mazerunnerV5.4.3b
```

Use CMake 3.20 or newer. The [build and reproduction guide](../03-Build-Test-and-Reproduce.md) includes direct compiler commands, Apple Silicon macOS instructions, Windows CMD instructions, dependency troubleshooting, and the full validation procedure.

| Control | Action |
|---|---|
| Mouse, Tab, Enter | Configure and start a game |
| WASD or arrow keys | Move the player (hold to keep walking) |
| G | Toggle automatic solving |
| Esc | Pause or resume (never quits) |
| Mouse wheel, +, − | Zoom |
| Home | Fit the maze to the viewport |
| Q | Quit |

To build the simulator without graphical libraries:

```bash
cmake -S . -B build/headless -DCMAKE_BUILD_TYPE=Release -DMAZERUNNER_BUILD_GRAPHICS=OFF
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
./build/headless/bin/maze-headless --simulate 33 33 5 42 60000 --difficulty medium --quiet
```

Here `42` is the random seed, which makes the run reproducible with the same build and inputs; `60000` is the simulated time limit in milliseconds.

## Development kit

[Download the complete V5.4.3b developer kit](https://github.com/LeEz1219/MazeRunner/raw/refs/heads/main/dist/MazeRunner-V5.4.3b-Documentation-and-Validation.zip). The archive includes the three guides, production source, CMake project, tests, raw results, 18 screenshots, reproduction scripts and integrity checks.

| File or directory | Contents |
|---|---|
| [src/mazerunnerV5.4.3b.cpp](../src/mazerunnerV5.4.3b.cpp) | Complete game source with embedded sprites and font |
| [CMakeLists.txt](../CMakeLists.txt) | Graphical, headless, portable regression, and EGL render/app test targets |
| [01-Source-Line-by-Line.md](../01-Source-Line-by-Line.md) | Line-by-line reference for all 3,631 physical source lines |
| [02-AI-Graphics-and-Redesign.md](../02-AI-Graphics-and-Redesign.md) | AI mechanics, measured escape rates, graphics, UI design, and redesign guidance |
| [03-Build-Test-and-Reproduce.md](../03-Build-Test-and-Reproduce.md) | Platform-specific compilation and reproduction instructions |
| [tests/](../tests/) | Gameplay, director, difficulty, V5.4.2 compatibility and V5.4.3b stalker behavior, UI, rendering, and application tests |
| [scripts/](../scripts/) | Validation, CMake, and calibration runners; documentation audit; package integrity verifier |
| [artifacts/results/](../artifacts/results/) | Recorded commands, environment, test logs, CMake record, and calibration results |
| [artifacts/screenshots/](../artifacts/screenshots/) | Eighteen PNG captures |
| [manifest.json](../manifest.json), [SHA256SUMS](../SHA256SUMS) | Package metadata and payload checksums |

The three numbered guides, `manifest.json`, `SHA256SUMS`, and everything under `artifacts/` were regenerated for V5.4.3b. The README lives under `.github/` so the verifier's exactly-three-guides check passes. `.gitattributes` keeps packaged file bytes identical across platforms, so the checksums hold on any checkout.

Verify the package before editing files or regenerating results, using Python 3.9 or newer:

```bash
python3 scripts/verify_package.py
```

The Linux validation runner rebuilds and reruns the supplied checks:

```bash
python3 scripts/run_validation.py --all
```

This command regenerates files under `artifacts/`, so use a separate working copy to retain the published reference results. `scripts/run_cmake_validation.py` records a real CMake/CTest run, and `scripts/run_calibration.py` re-measures auto-solver escape rates. After an intended change, `scripts/audit_documentation.py` re-records documentation coverage, `manifest.json`, and `SHA256SUMS`. See the reproduction guide for the Linux EGL/Mesa requirements and options for running only the portable checks.

## Recorded validation

The Linux validation covers the headless CLI, deterministic gameplay and UI regressions, ASan/UBSan and offscreen OpenGL. Ubuntu CI builds both the desktop and headless targets. [Ubuntu CMake run](https://github.com/LeEz1219/MazeRunner/actions/runs/44462877518) · [Direct validation](../artifacts/results/validation-summary.json)

Across 2,400 five-monster calibration games on 21×21 and 31×31, escapes were Low 739/800 (92.38%), Medium 526/800 (65.75%), High 290/800 (36.25%). The full 4,740-game data includes the ten-monster presets, rectangular setup default and fresh-seed holdouts. [All outcomes](../artifacts/results/calibration-runs.csv) · [Cohort results](../artifacts/results/calibration.json)
