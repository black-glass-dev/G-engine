#!/bin/sh
# G-Engine — tools/test.sh
# Builds the PC version and runs every test (no screen or graphics card needed).
#
#   sh tools/test.sh            engine tests
#   sh tools/test.sh --asan     the same with memory-error and undefined-behaviour checking
#   sh tools/test.sh --web      engine tests, then the web build and the browser test
#                               (needs `sh tools/setup.sh --browser-test` once)
set -e
cd "$(dirname "$0")/.."
. tools/env.sh

preset=native
web=0
for arg in "$@"; do
    case "$arg" in
        --asan) preset=native-asan ;;
        --web) web=1 ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

cmake --preset "$preset" >/dev/null
cmake --build "build/$preset"
"./build/$preset/tests/ge_tests" | grep -v '^\[ ok \]'

if [ "$web" = 1 ]; then
    sh tools/build_web.sh
    port="${GE_TEST_PORT:-8137}"
    GE_SERVE_QUIET=1 python3 tools/serve.py "$port" &
    server=$!
    trap 'kill "$server" 2>/dev/null' EXIT
    sleep 1
    node tools/web_test.mjs "http://localhost:$port/"
fi
