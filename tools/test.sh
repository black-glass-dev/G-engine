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

# Agent CLI smoke: a tiny game created, checked, played headless and packed —
# and the same input twice must give the exact same result (deterministic replay).
smoke="$(mktemp -d)"
"./build/$preset/bin/ge-game" new "$smoke/game" >/dev/null
"./build/$preset/bin/ge-game" check "$smoke/game" >/dev/null
"./build/$preset/bin/ge-game" simulate "$smoke/game" --seconds 2 --input "0.5:right:1,1:jump:0.2" > "$smoke/a.json"
"./build/$preset/bin/ge-game" simulate "$smoke/game" --seconds 2 --input "0.5:right:1,1:jump:0.2" > "$smoke/b.json"
cmp "$smoke/a.json" "$smoke/b.json"
"./build/$preset/bin/ge-game" pack "$smoke/game" -o "$smoke/game.json" >/dev/null
rm -rf "$smoke"
echo "[ ok ] agent CLI smoke (deterministic replay)"

if [ "$web" = 1 ]; then
    sh tools/build_web.sh
    port="${GE_TEST_PORT:-8137}"
    GE_SERVE_QUIET=1 python3 tools/serve.py "$port" &
    server=$!
    trap 'kill "$server" 2>/dev/null' EXIT
    sleep 1
    node tools/web_test.mjs "http://localhost:$port/"
fi
