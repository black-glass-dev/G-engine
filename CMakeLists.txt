cmake_minimum_required(VERSION 3.22)

project(Lumen2D VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(LUMEN_BUILD_TESTS "Build unit tests" ON)
option(LUMEN_BUILD_AGENTKIT "Build the lumen CLI (AgentKit)" ON)
option(LUMEN_BUILD_PLAYER "Build the standalone player (lumen_player)" ON)

include(CTest)

# SDL3 (Zlib). Android passes an existing SDL3 source tree and builds it as a shared library,
# so we only fetch it when no SDL3 target exists yet.
if(NOT TARGET SDL3::SDL3)
  include(FetchContent)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.20
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(SDL3)
endif()

add_subdirectory(engine)

if(LUMEN_BUILD_PLAYER)
  add_subdirectory(player)
endif()

if(LUMEN_BUILD_AGENTKIT)
  add_subdirectory(agentkit)
endif()

if(LUMEN_BUILD_TESTS AND BUILD_TESTING)
  add_subdirectory(tests)
endif()
