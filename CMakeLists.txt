cmake_minimum_required(VERSION 3.22)
project(GEngine VERSION 0.1.0 LANGUAGES C CXX)

# ------------------------------------------------------------------------------------------
# G-Engine — a 2D game engine for mobile, written in C++.
#   Platforms:  web     (WebAssembly + WebGL2; also the live preview)   -> cmake --preset web
#               android (NDK + OpenGL ES 3)                             -> see ROADMAP.md, phase 2
#               headless (no window; unit tests + simulation on a PC)   -> cmake --preset native
# ------------------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(GE_BUILD_TESTS "Build the unit tests (headless builds only)" ON)
option(GE_BUILD_DEMO "Build the demo game" ON)
option(GE_SANITIZE "Enable AddressSanitizer + UndefinedBehaviorSanitizer (native builds)" OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "WASI")
    set(GE_PLATFORM "web")
elseif (ANDROID)
    set(GE_PLATFORM "android")
else()
    set(GE_PLATFORM "headless")
endif()
message(STATUS "G-Engine platform: ${GE_PLATFORM} (${CMAKE_BUILD_TYPE})")

if (GE_SANITIZE AND GE_PLATFORM STREQUAL "headless")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(third_party)
add_subdirectory(engine)

if (GE_BUILD_DEMO)
    add_subdirectory(games/demo)
endif()

if (GE_BUILD_TESTS AND GE_PLATFORM STREQUAL "headless")
    enable_testing()
    add_subdirectory(tests)
endif()
