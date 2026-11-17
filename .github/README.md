# MazeRunner V5.4.1

MazeRunner is a C++17 maze game with an in-game setup screen, pixel-art characters, raised gray walls on a black background, and a headless simulator for repeatable AI experiments. Play manually or enable the automatic solver while roaming monsters and an adaptive director respond to movement.

![MazeRunner setup screen](../artifacts/screenshots/setup.png)

The setup screen offers Low, Medium, and High difficulty, a configurable monster count, and three maze presets: **Small 33 × 33**, **Standard 63 × 63**, and **Large 101 × 101**. The game includes ten monster sprite variants, full-route visualization, and an automatic solver that can wait, take alternate routes, and backtrack. Player and monster state logs remain available in the terminal.

![MazeRunner game view](../artifacts/screenshots/game.png)

## Build and play

On Ubuntu, install the compiler and graphical development dependencies, then run these commands from the repository root:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libglfw3-dev libglew-dev libgl1-mesa-dev pkg-config
cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --parallel
ctest --test-dir build/cmake --output-on-failure
./build/cmake/bin/mazerunnerV5.4.1
```

Use CMake 3.20 or newer. The [build and reproduction guide](../03-Build-Test-and-Reproduce.md) includes direct compiler commands, Apple Silicon macOS instructions, Windows CMD instructions, dependency troubleshooting, and the full validation procedure.

| Control | Action |
|---|---|
| Mouse, Tab, Enter | Configure and start a game |
| WASD or arrow keys | Move the player |
| G | Toggle automatic solving |
| Esc | Pause or resume |
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

| File or directory | Contents |
|---|---|
| [src/mazerunnerV5.4.1.cpp](../src/mazerunnerV5.4.1.cpp) | Complete game source with embedded sprites and font |
| [CMakeLists.txt](../CMakeLists.txt) | Graphical, headless, and portable regression targets |
| [01-Source-Line-by-Line.md](../01-Source-Line-by-Line.md) | Detailed annotations for all 2,643 physical source lines |
| [02-AI-Graphics-and-Redesign.md](../02-AI-Graphics-and-Redesign.md) | AI mechanics, graphics, UI design, and redesign guidance |
| [03-Build-Test-and-Reproduce.md](../03-Build-Test-and-Reproduce.md) | Platform-specific compilation and reproduction instructions |
| [tests/](../tests/) | Gameplay, director, difficulty, UI, rendering, and application tests |
| [scripts/](../scripts/) | Validation runner and package integrity verifier |
| [artifacts/results/](../artifacts/results/) | Recorded commands, environment, test logs, and results |
| [artifacts/screenshots/](../artifacts/screenshots/) | Eighteen PNG captures |
| [manifest.json](../manifest.json), [SHA256SUMS](../SHA256SUMS) | Original package metadata and payload checksums |

All 90 files from `MazeRunner-V5.4.1-Documentation-and-Validation.zip` are preserved byte-for-byte. This repository adds this README, a `.gitignore`, and a `.gitattributes` file. The README is under `.github/` so the original verifier's three-guide check continues to pass. Git checkout preserves packaged file bytes across platforms.

Verify the original package before editing files or regenerating results, using Python 3.9 or newer:

```bash
python3 scripts/verify_package.py
```

The Linux validation runner rebuilds and reruns the supplied checks:

```bash
python3 scripts/run_validation.py --all
```

This command regenerates files under `artifacts/`, so use a separate working copy to retain the published reference results. See the reproduction guide for the Linux EGL/Mesa requirements and options for running only the portable checks.

## Recorded validation scope

The packaged results passed on a Debian 13 x86-64 host with GCC 14.2.0. They cover gameplay and UI regressions, AddressSanitizer and UndefinedBehaviorSanitizer checks, repeated preset/difficulty smoke cases, real offscreen OpenGL rendering, and application logic with injected GLFW callbacks.

The supplied CMake configure/build/CTest workflow was not executed in that environment; its results directory explicitly records direct-compiler checks instead. Native desktop startup, native macOS builds, and native Windows builds were not executed. The platform instructions and recorded logs describe these limits in detail.
