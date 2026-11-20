# MazeRunner V5.4.4

MazeRunner is a C++17 maze game with an in-game setup screen, pixel-art characters, raised gray walls on a black background, and a headless simulator for repeatable AI experiments. Play manually or enable the automatic solver while roaming monsters and an adaptive director respond to movement.

![MazeRunner setup screen](../artifacts/screenshots/setup.png)

The setup screen offers Low, Medium, and High difficulty, a configurable monster count, and three maze presets: **Small 33 × 33**, **Standard 63 × 63**, and **Large 101 × 101**. The game includes ten monster sprite variants, full-route visualization, and an automatic solver that can wait, take alternate routes, and backtrack. Player and monster state logs remain available in the terminal.

![MazeRunner game view](../artifacts/screenshots/game.png)

## What's new in V5.4.4

When Backstage begins, the lead chooses a leg whose every step increases corridor distance from its own clue. A second hunter can disengage after two sustained pressure samples. Withdrawers reserve different paths, continue outward on arrival when possible, and hold if there is no legal step. A fresh sighting still takes priority.

A searching investigator ranks fresh junction arms by corridor direction toward its clue, straight-line reach and the learned alcove preference. A glance has its own short memory. On Low and Medium it changes what the monster checks next without revealing hidden cells. The solver now simulates those turns and retreat continuations on copies of the pack.

The learned junction ambush now considers every eligible listener’s own clue. The nearest qualifying junction wins, with a deterministic cell-ID tie break, and a Cutter still has to reach it in time.

On the default Medium maze, seed 26 shows relief at 4.000 s, withdrawal at 21.800 s and escape at 30.800 s. [Encounter trace](../artifacts/results/director-relief.log) · [Dedicated wiki](https://github.com/LeEz1219/MazeRunner/wiki)

## Build and play

On Ubuntu, install the compiler and graphical development dependencies, then run these commands from the repository root:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libglfw3-dev libglew-dev libgl1-mesa-dev libegl-dev pkg-config
cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --parallel
ctest --test-dir build/cmake --output-on-failure
./build/cmake/bin/mazerunnerV5.4.4
```

Use CMake 3.20 or newer. The [build and reproduction guide](../04-Build-Test-and-Reproduce.md) includes direct compiler commands, Apple Silicon macOS instructions, Windows CMD instructions, dependency troubleshooting, and the full validation procedure.

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

[Download the complete V5.4.4 developer kit](https://github.com/LeEz1219/MazeRunner/raw/refs/heads/main/dist/MazeRunner-V5.4.4-Documentation-and-Validation.zip). The archive includes the four guides, production source, CMake project, tests, raw results, 18 screenshots, reproduction scripts and integrity checks.

| File or directory | Contents |
|---|---|
| [src/mazerunnerV5.4.4.cpp](../src/mazerunnerV5.4.4.cpp) | Complete game source with embedded sprites and font |
| [CMakeLists.txt](../CMakeLists.txt) | Graphical, headless, portable regression, and EGL render/app test targets |
| [01-SourceCode-Deepdive-and- Explanation.md](../01-SourceCode-Deepdive-and-%20Explanation.md) | Intermediate/graduate C++ guide to ownership, scheduling, invariants and all 194 named functions and entry points |
| [02-AI-Design-Deepdive.md](../02-AI-Design-Deepdive.md) | Monster senses, pack learning, Director pacing, solver forecasts, graph proofs, runtime and measured balance |
| [03-Graphics-Design.md](../03-Graphics-Design.md) | Procedural walls, sprites, projection, UI, bitmap type and a Figma-to-C++ asset handoff |
| [04-Build-Test-and-Reproduce.md](../04-Build-Test-and-Reproduce.md) | Platform-specific compilation, testing, screenshot reproduction and release verification |
| [tests/](../tests/) | Gameplay, director, difficulty, V5.4.2 compatibility and V5.4.4 stalker behavior, UI, rendering, and application tests |
| [scripts/](../scripts/) | Validation, CMake, and calibration runners; documentation audit; package integrity verifier |
| [artifacts/results/](../artifacts/results/) | Recorded commands, environment, test logs, CMake record, and calibration results |
| [artifacts/screenshots/](../artifacts/screenshots/) | Eighteen PNG captures |
| [manifest.json](../manifest.json), [SHA256SUMS](../SHA256SUMS) | Package metadata and payload checksums |

The four numbered guides, source, tests and evidence are included in the package manifest. The checkout preserves file bytes across platforms, so the checksums hold on any checkout.

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

The Linux validation covers the headless CLI, deterministic gameplay and UI regressions, ASan/UBSan and offscreen OpenGL. Ubuntu CI builds both the desktop and headless targets. [Ubuntu CMake run](https://github.com/LeEz1219/MazeRunner/actions/runs/44822469091) · [Direct validation](../artifacts/results/validation-summary.json)

Across 2,400 five-monster calibration games, escape counts were Low 748/800 (93.50%), Medium 532/800 (66.50%), High 262/800 (32.75%). The 4,740-game dataset also includes the ten-monster presets, rectangular setup default and independent holdout seeds. [All outcomes](../artifacts/results/calibration-runs.csv) · [Cohort results](../artifacts/results/calibration.json)
