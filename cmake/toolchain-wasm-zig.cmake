# CMake toolchain: WebAssembly (wasm32-wasi) with the Zig compiler.
#
# Zig ships clang, libc and libc++ for WebAssembly in one small download, so the web build needs
# nothing but:   pip install cmake ninja ziglang
# (Emscripten is not required: the little JavaScript the engine needs lives in web/engine.js.)
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

set(CMAKE_C_COMPILER "${CMAKE_CURRENT_LIST_DIR}/zig/zig-cc")
set(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_LIST_DIR}/zig/zig-c++")
set(CMAKE_AR "${CMAKE_CURRENT_LIST_DIR}/zig/zig-ar" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${CMAKE_CURRENT_LIST_DIR}/zig/zig-ranlib" CACHE FILEPATH "" FORCE)
set(CMAKE_C_COMPILER_TARGET wasm32-wasi)
set(CMAKE_CXX_COMPILER_TARGET wasm32-wasi)

# CMake's compiler checks must not try to run or fully link a program for the target.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# One fixed optimisation level: Zig rebuilds its libc for every new flag set, which is slow.
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
