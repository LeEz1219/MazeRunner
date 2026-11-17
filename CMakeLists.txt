cmake_minimum_required(VERSION 3.20)
project(MazeRunner VERSION 5.4.1 LANGUAGES CXX)

# Configure out of source: cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release
option(MAZERUNNER_BUILD_GRAPHICS "Build the GLFW/OpenGL game" ON)
option(MAZERUNNER_BUILD_HEADLESS "Build the simulator without graphics dependencies" ON)
option(MAZERUNNER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
include(CTest) # Defines BUILD_TESTING and enables CTest when requested.
find_package(Threads REQUIRED)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration" FORCE)
endif()

# Accept either the extracted documentation bundle or a standalone source/file pair.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/mazerunnerV5.4.1.cpp")
    set(_maze_default_source "${CMAKE_CURRENT_SOURCE_DIR}/src/mazerunnerV5.4.1.cpp")
else()
    set(_maze_default_source "${CMAKE_CURRENT_SOURCE_DIR}/mazerunnerV5.4.1.cpp")
endif()
set(MAZERUNNER_SOURCE "${_maze_default_source}" CACHE FILEPATH "MazeRunner V5.4.1 source file")
if(NOT EXISTS "${MAZERUNNER_SOURCE}")
    message(FATAL_ERROR "Source not found: ${MAZERUNNER_SOURCE}. Place mazerunnerV5.4.1.cpp beside CMakeLists.txt or under src/, or set MAZERUNNER_SOURCE.")
endif()

# Keep output paths identical for Makefiles, Ninja, and multi-configuration IDEs.
function(maze_configure_target target output_name)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_link_libraries(${target} PRIVATE Threads::Threads)
    set_target_properties(${target} PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "${output_name}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${config}" config_upper)
        set_target_properties(${target} PROPERTIES
            "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/bin")
    endforeach()
    if(WIN32)
        target_compile_definitions(${target} PRIVATE NOMINMAX)
    endif()
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(MAZERUNNER_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(MAZERUNNER_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

if(MAZERUNNER_BUILD_GRAPHICS)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 3.3 CONFIG QUIET)
    if(TARGET glfw)
        set(_maze_glfw glfw)
    elseif(TARGET glfw3::glfw)
        set(_maze_glfw glfw3::glfw)
    elseif(TARGET glfw3)
        set(_maze_glfw glfw3)
    else()
        # Distribution packages without a CMake config may still supply glfw3.pc.
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(MAZERUNNER_GLFW QUIET IMPORTED_TARGET glfw3>=3.3)
        endif()
        if(NOT TARGET PkgConfig::MAZERUNNER_GLFW)
            message(FATAL_ERROR "GLFW 3.3+ development files were not found. Install GLFW/GLEW/OpenGL development packages, set CMAKE_PREFIX_PATH, or use -DMAZERUNNER_BUILD_GRAPHICS=OFF for a headless build.")
        endif()
        set(_maze_glfw PkgConfig::MAZERUNNER_GLFW)
    endif()

    if(TARGET GLEW::GLEW)
        set(_maze_glew GLEW::GLEW)
    elseif(TARGET GLEW::glew)
        set(_maze_glew GLEW::glew)
    elseif(TARGET GLEW::glew_s)
        set(_maze_glew GLEW::glew_s)
    else()
        message(FATAL_ERROR "GLEW was found but did not provide a supported imported target.")
    endif()

    # Intentionally retain the console on Windows so player/monster logs remain visible.
    add_executable(mazerunner "${MAZERUNNER_SOURCE}")
    maze_configure_target(mazerunner "mazerunnerV5.4.1")
    target_link_libraries(mazerunner PRIVATE ${_maze_glfw} ${_maze_glew} OpenGL::GL)
    get_target_property(_maze_glfw_type ${_maze_glfw} TYPE)
    if(WIN32 AND _maze_glfw_type STREQUAL "SHARED_LIBRARY")
        target_compile_definitions(mazerunner PRIVATE GLFW_DLL)
    endif()
endif()

if(MAZERUNNER_BUILD_HEADLESS)
    add_executable(maze-headless "${MAZERUNNER_SOURCE}")
    maze_configure_target(maze-headless "maze-headless")
    target_compile_definitions(maze-headless PRIVATE MAZERUNNER_HEADLESS)
endif()

if(BUILD_TESTING)
    # This entry point includes the production source and the other portable suites.
    # Do not add MAZERUNNER_SOURCE as a second translation unit here.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_tests.cpp")
        get_filename_component(_maze_selected_source "${MAZERUNNER_SOURCE}" REALPATH)
        get_filename_component(_maze_test_source "${CMAKE_CURRENT_SOURCE_DIR}/src/mazerunnerV5.4.1.cpp" REALPATH)
        if(NOT _maze_selected_source STREQUAL _maze_test_source)
            message(FATAL_ERROR "Bundled tests include src/mazerunnerV5.4.1.cpp. Use that source or set BUILD_TESTING=OFF when selecting a different MAZERUNNER_SOURCE.")
        endif()
        add_executable(maze-regressions tests/ui_tests.cpp)
        maze_configure_target(maze-regressions "maze-regressions")
        add_test(NAME gameplay-and-ui COMMAND maze-regressions)
        set_tests_properties(gameplay-and-ui PROPERTIES TIMEOUT 300)
    else()
        message(STATUS "Portable test sources are absent; building the standalone game/simulator only.")
    endif()

    if(TARGET maze-headless)
        add_test(NAME cli-help COMMAND maze-headless --help)
        set_tests_properties(cli-help PROPERTIES
            PASS_REGULAR_EXPRESSION "Maze Runner V5\\.4\\.1" TIMEOUT 30)
        foreach(size IN ITEMS 33 63 101)
            foreach(difficulty IN ITEMS low medium high)
                add_test(NAME "smoke-${size}-${difficulty}" COMMAND maze-headless
                    --simulate ${size} ${size} 10 42 1000 --difficulty ${difficulty} --quiet)
                set_tests_properties("smoke-${size}-${difficulty}" PROPERTIES
                    PASS_REGULAR_EXPRESSION "Maze Runner V5\\.4\\.1.*${size}x${size}.*monsters=10"
                    TIMEOUT 300)
            endforeach()
        endforeach()
    endif()
endif()

if(NOT TARGET mazerunner AND NOT TARGET maze-headless AND NOT TARGET maze-regressions)
    message(FATAL_ERROR "No build targets selected. Enable graphics, headless, or the bundled tests.")
endif()
