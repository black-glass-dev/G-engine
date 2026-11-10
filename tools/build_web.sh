#!/bin/sh
# G-Engine — tools/build_web.sh
# Builds the engine for the browser (WebAssembly) and puts it next to the web page.
#   once:   sh tools/setup.sh
#   then:   python3 tools/serve.py        and open the address it prints
set -e
cd "$(dirname "$0")/.."
. tools/env.sh
cmake --preset web
cmake --build build/web
cp build/web/games/demo/g-engine.wasm web/g-engine.wasm
echo "web/g-engine.wasm  ($(wc -c < web/g-engine.wasm) bytes)"
