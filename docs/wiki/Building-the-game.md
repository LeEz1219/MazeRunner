# Build on Ubuntu, Apple Silicon or Windows CMD

[MazeRunner V5.4.3a wiki](Home.md)

## CMake build on all three platforms

The repository includes [CMakeLists.txt](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/CMakeLists.txt). Use **CMake 3.20 or newer** and a C++17 compiler. CMake generates the build system, `cmake --build` compiles its targets, and CTest runs the registered checks. These commands are an alternative to the direct compiler instructions below, not an additional required compilation pass.

The supplied configuration builds `mazerunner` (graphical game), `maze-headless` (simulator), and `maze-regressions` (the portable gameplay/UI suite). On Linux, when `libEGL` and `libGL` are found, it also builds `maze-render-tests` and `maze-app-tests`, the real-OpenGL offscreen suites. Executables go under the selected build directory's `bin/`. The graphical executable is named `mazerunnerV5.4.3a` (from the full release label), with `.exe` appended on Windows. The other executable names match their target names. CMake keeps the numeric project version at 5.4.3 and appends the lowercase suffix in `MAZERUNNER_RELEASE`. This full label supplies `MAZERUNNER_VERSION` in the game, window title, logs and CLI help. The Windows game deliberately retains its console so state logs remain visible.

### Ubuntu: configure, build, test, run

From the extracted package root:

```bash
sudo apt update
sudo apt install cmake ninja-build build-essential libglfw3-dev libglew-dev libgl1-mesa-dev libegl-dev pkg-config
cmake --version
cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --parallel
ctest --test-dir build/cmake --output-on-failure
./build/cmake/bin/mazerunnerV5.4.3a --seed 42
```

If CMake is older than 3.20, update it before configuring. The supplied file locates OpenGL and GLEW through imported targets. It first looks for GLFW's CMake package; if that is unavailable, it tries `pkg-config` metadata for GLFW 3.3 or later. Failure to find development dependencies is a configuration error, not a reason to use the test-only GL headers for the desktop build.

### Apple Silicon macOS: native arm64 build

Install Apple's command-line tools and native Homebrew as described in the macOS section below, then run in Terminal from the package root:

```bash
brew install cmake ninja glfw glew pkgconf
cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build/cmake --parallel
ctest --test-dir build/cmake --output-on-failure
./build/cmake/bin/mazerunnerV5.4.3a --seed 42
```

The prefix helps CMake locate the native Homebrew dependencies, and `arm64` avoids silently producing an Intel build. `OpenGL::GL` supplies the platform-specific framework linkage; do not add Linux's `-lGL` manually. This still produces a command-line executable using installed libraries, not a bundled or signed `.app`.

### Windows CMD: UCRT64 with Ninja

Install/update MSYS2 and its GCC/GLFW/GLEW packages using the Windows section below. Then, in CMD, install CMake and Ninja from that same UCRT64 environment:

```bat
set "MSYSTEM=UCRT64"
"C:\msys64\usr\bin\bash.exe" -lc "pacman -S --needed mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja"
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
cd /d "%USERPROFILE%\Downloads\MazeRunner"
cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build/cmake --parallel
ctest --test-dir build/cmake --output-on-failure
build\cmake\bin\mazerunnerV5.4.3a.exe --seed 42
```

These are CMD commands, including the explicit Bash invocation for package management. Selecting `Ninja` and the UCRT64 compiler avoids accidentally combining Visual Studio/MSVC with MinGW libraries. Keep the UCRT64 `bin` directory on PATH while running so the matching GLFW, GLEW, and GCC runtime DLLs can be found. CMake does not automatically copy those DLLs into the output directory; the existing Windows DLL instructions still apply.

### Options, standalone source, and test scope

| CMake cache option | Default | Effect |
|---|---|---|
| `MAZERUNNER_BUILD_GRAPHICS` | `ON` | Build the GLFW/GLEW/OpenGL desktop game; disable to remove those dependency searches |
| `MAZERUNNER_BUILD_HEADLESS` | `ON` | Build the simulation executable with `MAZERUNNER_HEADLESS` |
| `BUILD_TESTING` | `ON` | Register the portable regression suite, CLI help, and nine preset/profile smoke cases when their targets/sources are present |
| `MAZERUNNER_EGL_TESTS` | `ON` | On Linux, also build and register the real-OpenGL render and application suites when `libEGL`/`libGL` are found; skipped with a status message otherwise |
| `MAZERUNNER_WARNINGS_AS_ERRORS` | `OFF` | Add `/WX` or `-Werror` for stricter compiler checking |
| `MAZERUNNER_SOURCE` | Detected file | Use `src/mazerunnerV<version>.cpp` from the project version, otherwise the same filename beside `CMakeLists.txt`; an explicit FILEPATH override is supported |
| `CMAKE_BUILD_TYPE` | `Release` for single-configuration generators | Choose optimization/debug settings; it does not change game difficulty |

For a dependency-free headless build on Linux or macOS with Ninja installed:

```bash
cmake -S . -B build/headless -G Ninja -DCMAKE_BUILD_TYPE=Release -DMAZERUNNER_BUILD_GRAPHICS=OFF
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
./build/headless/bin/maze-headless --simulate 33 33 5 42 60000 --difficulty medium --quiet
```

On Windows CMD, retain the explicit UCRT64 compiler choice:

```bat
cmake -S . -B build/headless -G Ninja -DCMAKE_BUILD_TYPE=Release -DMAZERUNNER_BUILD_GRAPHICS=OFF -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
build\headless\bin\maze-headless.exe --simulate 33 33 5 42 60000 --difficulty medium --quiet
```

On Linux with EGL available, the repository registers **13 CTest tests**: one portable regression entry point, the `render-egl` and `app-egl` real-OpenGL suites, one CLI help check, and nine size/difficulty smoke cases. Without EGL (macOS, Windows, or Linux without the libraries) it registers 11. The EGL suites write their screenshots under the build directory, not into `artifacts/`. The separate Python validation runner still provides repeated smoke comparisons and sanitizers, which CTest does not run. To build only the desktop game, configure with `-DMAZERUNNER_BUILD_HEADLESS=OFF -DBUILD_TESTING=OFF` and retain graphics enabled.

If you download only the `.cpp` and `CMakeLists.txt`, place them in the same folder. Missing regression sources are reported and skipped; the game/headless targets can still build. In the full repository, portable tests directly include `src/mazerunnerV5.4.3a.cpp`. Selecting a different `MAZERUNNER_SOURCE` therefore requires `-DBUILD_TESTING=OFF`; this prevents a misleading result where the tests exercise another source file. No source file is downloaded automatically.

Use a fresh build directory when changing compiler, CPU architecture, or generator. A stale CMake cache can retain the previous toolchain/dependency paths. CMake's multi-configuration generators also accept `cmake --build build/cmake --config Release` and `ctest --test-dir build/cmake -C Release --output-on-failure`; the supported command recipes above use single-configuration Ninja. The file contains MSVC warning handling, but no native MSVC build was executed or established by this package.

Ubuntu CI executed `python3 scripts/run_cmake_validation.py --generator Ninja --jobs 2`. Its Release build included the native desktop binary, CLI and EGL harnesses. All 13 CTest cases passed. For an installation without GLFW/GLEW development headers, pass `--headless` and a different `--build-dir`. [Configure log](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/cmake/configure.log) · [Build log](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/cmake/build.log) · [CTest log](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/cmake/ctest.log) · [Hosted run](https://github.com/LeEz1219/MazeRunner/actions/runs/44216985629)

## Ubuntu Linux: direct compiler build

These instructions use Ubuntu's native packages and Bash. A current desktop installation with working graphics drivers is assumed. The Ubuntu package index lists the GLFW and GLEW development packages used here; GLFW's official build guide explains `pkg-config`-based linking. See the primary references at the end.

### Step 1 — install the compiler and graphics development packages

Open Terminal and run:

```bash
sudo apt update
sudo apt install build-essential libglfw3-dev libglew-dev libgl1-mesa-dev pkg-config
```

`build-essential` provides the GNU C++ toolchain. The `-dev` packages supply headers and linker metadata; merely having a runtime library is insufficient to compile. If Ubuntu reports that a package from Universe is unavailable, enable that repository and retry the installation:

```bash
sudo add-apt-repository universe
sudo apt update
sudo apt install libglfw3-dev libglew-dev
```

On a minimal image where `add-apt-repository` is missing, install `software-properties-common` first. A normal desktop installation commonly already has the repository enabled.

### Step 2 — enter the extracted package and verify dependencies

Replace the example directory with your actual extraction path; retain quotes around paths containing spaces.

```bash
cd "$HOME/Downloads/MazeRunner"
mkdir -p build
g++ --version
pkg-config --modversion glfw3 glew
sha256sum src/mazerunnerV5.4.3a.cpp
```

Compare the hash with the value above. `pkg-config` should print versions for both libraries. If it cannot find `glfw3` or `glew`, resolve that installation issue before compiling.

### Step 3 — compile the desktop game

```bash
g++ src/mazerunnerV5.4.3a.cpp -o build/mazerunnerV5.4.3a -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic $(pkg-config --cflags glfw3 glew) $(pkg-config --libs glfw3 glew) -lGL -pthread
```

The shell expands the two `pkg-config` expressions into this installation's include/link flags. Source/object inputs precede libraries so the linker can resolve references correctly. `-pthread` supports the asynchronous generation worker, the frame limiter, and the standard threading runtime. `-O2 -DNDEBUG` is a release-style configuration; it is not a difficulty setting. No `-march=native` is used, avoiding an unnecessary CPU-specific instruction requirement.

If package metadata is unavailable but the normal Ubuntu development packages are installed, the equivalent explicit-library command is:

```bash
g++ src/mazerunnerV5.4.3a.cpp -o build/mazerunnerV5.4.3a -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -lglfw -lGLEW -lGL -pthread
```

Use either command, not both as successive mandatory steps. Successful compilation normally returns to the prompt without output. The result is a Linux executable, not a Windows `.exe` or macOS application.

### Step 4 — launch and inspect

```bash
./build/mazerunnerV5.4.3a
```

Select difficulty, a preset or custom dimensions, and monster count. Start opens a loading screen, then the maze. Monsters stay still until your first move or until you press G; the HUD says "MOVE OR PRESS G TO START" until then. Hold a movement key to keep walking. Esc pauses and resumes but never quits; use Q or the QUIT button. For a repeatable initial maze and fixed-seed replays:

```bash
./build/mazerunnerV5.4.3a --seed 42 --difficulty medium
```

`42` is simply an example seed, not a special balance parameter or a required value. The difficulty option preselects the card; the user can still change it in setup. To preserve detailed diagnostics while playing:

```bash
./build/mazerunnerV5.4.3a --seed 42 --verbose 2>&1 | tee desktop-session.log
```

`--quiet` suppresses normal state logs, while `--verbose` adds individual monster-step reports. Omit `--quiet` if you want to inspect pursuit and player state. To inspect shared-library resolution:

```bash
ldd build/mazerunnerV5.4.3a
```

Investigate any `not found` entry before debugging gameplay. Launching over SSH without a usable display is different from building successfully; use headless mode for simulation-only work in that environment.

## macOS on M-series Apple Silicon

This path builds a native **arm64** executable with Apple's Clang and Homebrew dependencies. It does not produce an Intel binary, a universal binary, or a self-contained `.app` bundle. The source already requests a forward-compatible OpenGL core context under `__APPLE__`; that is not an edit you need to make.

GLFW documents Apple's core-profile context requirements and OpenGL limitations. OpenGL is deprecated on macOS, so treat the local graphical launch and driver check as part of verification rather than inferring it from a successful Linux test. The current source targets OpenGL 3.3/GLSL 3.30 and does not require a newer GL feature level.

### Step 1 — use a native Apple Silicon Terminal and install compiler tools

In Terminal:

```bash
uname -m
xcode-select --install
```

`uname -m` should report `arm64` for this workflow. If it reports `x86_64` on an M-series Mac, the terminal may be running under Rosetta; use a native terminal before installing a second, incompatible dependency set. Complete Apple's installer dialog if offered. If a suitable Xcode/toolchain is already installed, the command may report that tools are already available.

Verify the active compiler and developer directory:

```bash
xcrun clang++ --version
xcode-select -p
```

Apple's Command Line Tools package is sufficient for this command-line build; full Xcode is not required solely to compile this file.

### Step 2 — install Homebrew and the graphics libraries

If Homebrew is absent, use the current installer from [brew.sh](https://brew.sh/) or its official `.pkg` option. The published shell installer is:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow its displayed installation steps. The standard Apple Silicon prefix is `/opt/homebrew`. For the current terminal session:

```bash
eval "$(/opt/homebrew/bin/brew shellenv)"
brew install glfw glew pkgconf
brew --prefix
pkg-config --modversion glfw3 glew
```

Follow Homebrew's printed shell-profile instructions if you want its tools available in future terminals. `pkgconf` supplies the `pkg-config` command. Keep the compiler target, GLFW, and GLEW architectures consistent; do not combine an Intel `/usr/local` library set with an arm64 compilation.

### Step 3 — compile for arm64

```bash
cd "$HOME/Downloads/MazeRunner"
mkdir -p build
shasum -a 256 src/mazerunnerV5.4.3a.cpp
clang++ src/mazerunnerV5.4.3a.cpp -o build/mazerunnerV5.4.3a -std=c++17 -O2 -DNDEBUG -arch arm64 -Wall -Wextra -Wpedantic $(pkg-config --cflags glfw3 glew) $(pkg-config --libs glfw3 glew) -framework OpenGL -pthread
```

The OpenGL framework is the macOS counterpart of Linux's `-lGL`. The package metadata provides the library search paths and GLFW/GLEW linkage. If you need an explicit-path fallback for a standard Homebrew installation, use:

```bash
clang++ src/mazerunnerV5.4.3a.cpp -o build/mazerunnerV5.4.3a -std=c++17 -O2 -DNDEBUG -arch arm64 -I"$(brew --prefix)/include" -L"$(brew --prefix)/lib" -lglfw -lGLEW -framework Cocoa -framework OpenGL -framework IOKit -framework QuartzCore -pthread
```

This is an alternative to the metadata command. It is not necessary to edit system include paths or copy headers into the project.

### Step 4 — verify and run

```bash
file build/mazerunnerV5.4.3a
otool -L build/mazerunnerV5.4.3a
./build/mazerunnerV5.4.3a --seed 42 --difficulty medium
```

`file` should identify an arm64 Mach-O executable. `otool -L` shows the dynamic libraries/frameworks it expects. Run the manual checklist later in this document, including Retina pointer alignment and focus-loss pause. The game uses Control+A for numeric-field replacement; Command+A is not mapped by this source. Clicking a field already prepares it for replacement on the next digit.

A locally built executable may depend on libraries under the builder's Homebrew prefix. Sending only that executable does not automatically supply those libraries to another Mac. App bundling, dependency relocation, signing, and notarization are separate distribution work, beyond this source-compilation guide. The simplest designer workflow here is to build on the target Mac with its own native dependencies.

## Windows: compile and run from CMD

These instructions target **64-bit Intel/AMD Windows** using the MSYS2 **UCRT64 MinGW-w64** toolchain. The compiler produces a native Windows executable. This is not WSL, Cygwin, or an MSVC build. All commands presented below are entered in **Command Prompt (`cmd.exe`)**, not PowerShell. Package-management commands explicitly invoke MSYS2's Bash; the actual compiler/run commands remain ordinary CMD commands.

### Step 1 — install and update MSYS2

Download the x86-64 installer from [msys2.org](https://www.msys2.org/) and use the standard `C:\msys64` installation directory for these examples. If you choose another directory, replace it consistently in all commands. Open CMD and run:

```bat
set "MSYSTEM=UCRT64"
"C:\msys64\usr\bin\bash.exe" -lc "pacman -Syu"
```

Accept the package manager's prompts as appropriate. If a core update closes the MSYS2 process or asks you to restart, invoke the same update command again from a fresh CMD session, setting `MSYSTEM` again. Complete the full update before installing dependencies; MSYS2 is a rolling environment and does not support partial package upgrades.

Install the matching UCRT64 compiler and both graphics libraries:

```bat
"C:\msys64\usr\bin\bash.exe" -lc "pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glew"
```

Use the same `ucrt64` prefix for every dependency. Mixing `mingw64`, `ucrt64`, MSVC `.lib` files, or 32-bit objects can cause incompatible-runtime or link failures.

### Step 2 — select the compiler in CMD and enter the project

```bat
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
where g++
g++ --version
cd /d "%USERPROFILE%\Downloads\MazeRunner"
if not exist build mkdir build
certutil -hashfile src\mazerunnerV5.4.3a.cpp SHA256
```

The first `where g++` result should be `C:\msys64\ucrt64\bin\g++.exe`. The PATH change applies to the current CMD session; repeat it when opening a new one. `cd /d` changes drive as well as directory. Compare the source hash against the package value.

Confirm that the installed headers/import libraries exist:

```bat
dir C:\msys64\ucrt64\include\GLFW\glfw3.h
dir C:\msys64\ucrt64\include\GL\glew.h
dir C:\msys64\ucrt64\lib\libglfw3.dll.a
dir C:\msys64\ucrt64\lib\libglew32.dll.a
```

### Step 3 — compile the graphical `.exe`

Paste this as one CMD command:

```bat
g++ "src\mazerunnerV5.4.3a.cpp" -o "build\mazerunnerV5.4.3a.exe" -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -DNOMINMAX -DGLFW_DLL -I"C:\msys64\ucrt64\include" "C:\msys64\ucrt64\lib\libglfw3.dll.a" "C:\msys64\ucrt64\lib\libglew32.dll.a" -lopengl32 -pthread
```

The explicit `.dll.a` files are import libraries for the DLL versions; they avoid ambiguity with similarly named static archives. `GLFW_DLL` declares GLFW's DLL linkage. Do not add `GLEW_STATIC` to this DLL build. `NOMINMAX` prevents Windows header macros from colliding with C++ `std::min`/`std::max`. `opengl32` supplies the system GL linkage, while GLEW loads the additional context functions used by the game.

Do not add `-mwindows` to this primary command: retaining a console preserves the terminal logs used to inspect the player, monsters, and director. If distributing a later GUI-only build, provide another explicit logging destination before hiding that console.

CMD does not understand Bash's `$(pkg-config ...)`, `export`, or backslash line continuation. That is why this Windows command uses explicit paths and one line. A CMD multiline command would use `^`, but no continuation character is needed here.

### Step 4 — run with the correct DLL search path

From the same CMD session:

```bat
build\mazerunnerV5.4.3a.exe --seed 42 --difficulty medium
```

The PATH set above makes `glfw3.dll`, `glew32.dll`, and the compiler runtime DLLs available. For a diagnostic log file:

```bat
build\mazerunnerV5.4.3a.exe --seed 42 --verbose > desktop-session.log 2>&1
```

This redirects output to the file, so it will not also scroll in the CMD window. Inspect it afterward with `type desktop-session.log` or a text editor.

For a locally self-contained test folder, copy the dependency DLLs next to the `.exe` from the **same UCRT64 installation**. Common runtime dependencies include the following, but inspect your actual build rather than assuming this list is exhaustive:

```bat
copy "C:\msys64\ucrt64\bin\glfw3.dll" "build\"
copy "C:\msys64\ucrt64\bin\glew32.dll" "build\"
copy "C:\msys64\ucrt64\bin\libstdc++-6.dll" "build\"
copy "C:\msys64\ucrt64\bin\libgcc_s_seh-1.dll" "build\"
copy "C:\msys64\ucrt64\bin\libwinpthread-1.dll" "build\"
```

Use the installed toolchain to inspect imports:

```bat
objdump -p build\mazerunnerV5.4.3a.exe | findstr /C:"DLL Name"
objdump -p C:\msys64\ucrt64\bin\glfw3.dll | findstr /C:"DLL Name"
objdump -p C:\msys64\ucrt64\bin\glew32.dll | findstr /C:"DLL Name"
```

Inspect any additional non-system DLLs transitively as needed. Windows system DLLs such as `opengl32.dll` come from Windows/graphics drivers; do not substitute downloaded copies from unrelated sources. A successful link does not prove the installed graphics driver supports OpenGL 3.3. Test startup on the actual target machine, particularly under remote desktop or virtual-machine graphics.
