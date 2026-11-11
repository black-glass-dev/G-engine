# Native build for the Android APK: SDL3 (shared, required by SDLActivity) + liblumen + lumen_player.
cmake_minimum_required(VERSION 3.22)
project(LumenAndroid LANGUAGES C CXX)

if(NOT LUMEN_ROOT OR NOT LUMEN_SDL_DIR)
  message(FATAL_ERROR "LUMEN_ROOT and LUMEN_SDL_DIR must be passed from app/build.gradle.kts")
endif()

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${LUMEN_SDL_DIR} ${CMAKE_BINARY_DIR}/sdl3)

set(LUMEN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUMEN_BUILD_AGENTKIT OFF CACHE BOOL "" FORCE)
set(LUMEN_BUILD_PLAYER ON CACHE BOOL "" FORCE)
add_subdirectory(${LUMEN_ROOT} ${CMAKE_BINARY_DIR}/lumen)
