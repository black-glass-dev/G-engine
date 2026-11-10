# G-Engine — Agent Edition

You (an AI agent) can build complete games with this engine from the command line:
write a level picture and a few Kotha scripts, check them, play the game headless,
read what happened, repeat. No screen, no browser, no guessing.

A companion document in Bengali for people: [docs/KOTHA.md](docs/KOTHA.md).

## Build the tools (about a minute)

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native
# → build/native/bin/ge-game   (the agent CLI)
# → build/native/tests/ge_tests (the engine test suite)
```

`ge-game` is written in `tools/game_cli.cpp` (~350 lines) on top of the same engine
library the games run on. There is also `tools/mcp_server.py` (see below).

## A game folder

```
mygame/
  level.txt        the level picture — one character per tile
  খেলোয়াড়.kotha    the script of the player
  প্রহরী.kotha      the script of the guard
  কয়েন.kotha       …one file per thing, the file name IS the thing name
```

The level picture (legend: `#` wall · `.` empty · `P` player · `G` guard · `c` coin ·
`^` spike · `B` box · `D` door · `L` lamp):

```
########################
#......................#
#..c.c............c....#
#..####..........####..#
#.......L..............#
#..P.......c.......D...#
########################
```

Scripts are written in Kotha — Bengali or English sentences. One file's rules talk
about its own thing first ("I"):

```text
# কয়েন.kotha
খেলোয়াড়কে ছুঁলে মুছে যাও
```

```text
# প্রহরী.kotha
সবসময় খেলোয়াড়ের দিকে ছুটো
খেলোয়াড়কে ছুঁলে "ধরে ফেলেছি!" বলো
```

## The workflow

```sh
build/native/bin/ge-game new mygame            # 1. start from the starter folder
build/native/bin/ge-game check mygame          # 2. compile the scripts → JSON errors/notes
build/native/bin/ge-game simulate mygame --seconds 30 \
    --input "0.5:right:2,3:jump:0.2"           # 3. play it headless → JSON of what happened
build/native/bin/ge-game pack mygame -o mygame.json   # 4. bundle it into one file
```

### `check` output

```json
{"ok":false,"dir":"mygame","errors":[],
 "files":[{"file":"কয়েন.kotha","name":"কয়েন",
   "check":{"ok":false,"rules":1,
     "notes":[{"line":2,"bn":"খেলোয়াড়কে ছুঁলে ⇒ নিজে মুছে যাও","en":"on touching the player ⇒ remove myself"}],
     "errors":[{"line":3,"col":1,"bn":"…what went wrong…","en":"…"}]}}]}
```

Read `check.errors` — each has `line`, `col`, `bn`, `en`. `check.notes` echo every rule
back in plain words: use them to confirm the script does what you meant.

### `simulate` output

```json
{"ok":true,"dir":"mygame","seconds":30,"steps":1800,"state":"won",
 "player":{"x":19.5,"y":1.42},
 "vars":{"স্কোর":3},
 "said":["কে ওখানে?","ধরে ফেলেছি!"]}
```

`state` is `playing`, `won` or `lost`. `vars` are the game variables scripts set
(`স্কোর`/score, custom ones too). `said` is everything anyone said, in order.
Drive the game with `--input "t:button:hold,…"` (buttons: left right up down jump action).

### Common errors and how to fix them

- A plain line after a top-level line is an error — put start-of-level actions under
  `শুরুতে:` / `at start:`.
- Unknown words become names — if a typo suggests a real word (edit distance 3),
  the error offers the fix. Always apply it.
- `is at least` / `is at most` are not supported — use `>` / `<` / `=` (Bengali: বড়/ছোট/সমান).
- The state triggers (`খেলোয়াড় জিতলে`, `খেলোয়াড় হারলে`) fire once — write their
  consequences on the lines below.

## Talking to the engine from an MCP host

`tools/mcp_server.py` is a dependency-free [Model Context Protocol](https://modelcontextprotocol.io)
server over stdio (JSON-RPC 2.0). Point any MCP host at it:

```json
{ "command": "python3", "args": ["tools/mcp_server.py"] }
```

It exposes `gengine_new_game`, `gengine_check_game`, `gengine_simulate_game`,
`gengine_pack_game` and `kotha_help` (the language guide). Every result is the same
JSON as the CLI prints.

## Where things live

| Path | What |
| --- | --- |
| `engine/` | the engine: lexer, compiler, VM, physics, world, renderer |
| `engine/script/` | the Kotha language (compiler.h is the map) |
| `games/demo/` | the demo game — a worked example of a full game |
| `docs/KOTHA.md` | the language guide (Bengali + English) |
| `docs/RENDERING.md` | the lighting model |
| `ROADMAP.md` | what gets built next and how (research-based, big batches) |
| `tests/` | `ge_tests` — run it after engine changes |
| `tools/game_cli.cpp` | the agent CLI source |
| `tools/mcp_server.py` | this MCP server |

**This CLI is the main workbench of the project** (see `ROADMAP.md`, rule 2): every engine
feature is built and proven here first, and the same input always gives the same result
(deterministic replay is checked in `tools/test.sh`). When you add engine behaviour, add it
to `ge-game` too.

The same engine also ships as a web studio (`web/`, `sh tools/build_web.sh`) and as an
Android studio app (`android/`, built by GitHub Actions). Games you build with the CLI
can be loaded in the web studio (Import) and played on a phone.

Keep answers to the human in Bengali; keep code, JSON and file names exactly as here.
