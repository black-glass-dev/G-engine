#!/bin/sh
# G-Engine — tools/setup.sh
# Installs what is needed to build the engine: CMake, Ninja and Zig (the WebAssembly compiler).
# All three come from PyPI, so Python 3 with pip is all you need to have beforehand.
# For the PC build and the tests a C++20 compiler is needed as well (g++ 11+ or clang 14+).
#
#   sh tools/setup.sh                   build tools
#   sh tools/setup.sh --browser-test    ... and what the browser test needs (Node.js 18+; downloads a Chromium, ~70 MB)
set -e
cd "$(dirname "$0")/.."

if ! python3 -m pip --version >/dev/null 2>&1; then
    echo "Python 3 with pip is required (https://www.python.org/downloads/)." >&2
    exit 1
fi

# In a virtual environment: install there. Otherwise into the user's own folder; newer
# systems want --break-system-packages for that (nothing of the system is touched).
if python3 -c 'import sys; sys.exit(0 if sys.prefix != sys.base_prefix else 1)'; then
    python3 -m pip install --quiet cmake ninja ziglang
elif ! python3 -m pip install --quiet --user cmake ninja ziglang 2>/dev/null; then
    python3 -m pip install --quiet --user --break-system-packages cmake ninja ziglang
fi

. tools/env.sh
echo "cmake   $(cmake --version | head -n 1 | sed 's/cmake version //')"
echo "ninja   $(ninja --version)"
echo "zig     $(python3 -m ziglang version)"
if command -v c++ >/dev/null 2>&1; then
    echo "c++     $(c++ --version | head -n 1)"
else
    echo "c++     NOT FOUND - the browser build works without it, the PC build and the tests need g++ or clang"
fi

if [ "$1" = "--browser-test" ]; then
    if ! command -v npm >/dev/null 2>&1; then
        echo "Node.js with npm is required for the browser test (https://nodejs.org/)." >&2
        exit 1
    fi
    (cd tools && npm install --no-audit --no-fund)
fi

echo
echo "Ready.  Next:"
echo "  sh tools/build_web.sh && python3 tools/serve.py     build the web player and open it"
echo "  sh tools/test.sh                                     build the PC version and run the tests"
