#!/usr/bin/env python3
"""G-Engine MCP server — lets an AI agent build games with the engine.

Speaks the Model Context Protocol over stdio (newline-delimited JSON-RPC 2.0,
the standard transport). No dependencies beyond the Python standard library.
Everything is a thin wrapper over the `ge-game` agent CLI.

Wire it into any MCP host (Claude Code, Cursor, Arena agents, …) as:

    command: python3 tools/mcp_server.py

The server finds the repository around itself. If the `ge-game` binary is not
at build/native/bin/ge-game, point the GE_GAME_BIN environment variable at it
or build first:  cmake -S . -B build/native && cmake --build build/native
"""

import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GE_GAME = Path(os.environ.get("GE_GAME_BIN", str(ROOT / "build" / "native" / "bin" / "ge-game")))
KOTHA_DOC = ROOT / "docs" / "KOTHA.md"
PROTOCOL_VERSION = "2025-06-18"

TOOLS = [
    {
        "name": "gengine_new_game",
        "description": (
            "Create a new G-Engine game folder (the starter level picture and Kotha scripts). "
            "The folder is: level.txt (the level picture) plus one <thing>.kotha file per thing "
            "(the file name is the thing name: খেলোয়াড়.kotha, প্রহরী.kotha, কয়েন.kotha, …)."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {"dir": {"type": "string", "description": "folder to create"}},
            "required": ["dir"],
        },
    },
    {
        "name": "gengine_check_game",
        "description": (
            "Compile every Kotha script in a game folder and return JSON errors and notes. "
            "Errors have a line, a column and a Bengali and English message. Notes explain in "
            "plain words what each line of the script does."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {"dir": {"type": "string", "description": "the game folder"}},
            "required": ["dir"],
        },
    },
    {
        "name": "gengine_simulate_game",
        "description": (
            "Run a game headless (no window) and return JSON of what happened: final state "
            "(playing/won/lost), the player position, the game variables (like স্কোর) and "
            "everything anyone said. Optionally press buttons during the run."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "dir": {"type": "string", "description": "the game folder"},
                "seconds": {"type": "number", "description": "how long to play (default 30)"},
                "input": {
                    "type": "string",
                    "description": (
                        "button presses: \"t:button:hold,…\" — t = start time in seconds, "
                        "hold = how long to hold; buttons: left right up down jump action. "
                        "Example: \"0.5:right:2,3:jump:0.2\""
                    ),
                },
            },
            "required": ["dir"],
        },
    },
    {
        "name": "gengine_pack_game",
        "description": "Bundle a game folder into one JSON file (level + scripts) for sharing.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "dir": {"type": "string", "description": "the game folder"},
                "output": {"type": "string", "description": "output .json path (default <dir>.json)"},
            },
            "required": ["dir"],
        },
    },
    {
        "name": "kotha_help",
        "description": (
            "The Kotha language guide — how to write game rules in Bengali or English "
            "(\"যখন খেলোয়াড় কয়েন ছোঁয়, তখন কয়েন লুকাও। স্কোর ১ বাড়াও।\"). "
            "Read this before writing scripts."
        ),
        "inputSchema": {"type": "object", "properties": {}},
    },
]


def run_cli(args, timeout=120):
    if not GE_GAME.exists():
        return None, (
            "ge-game is not built yet. Build it with:\n"
            "  cmake -S . -B build/native && cmake --build build/native\n"
            "(or set GE_GAME_BIN to the binary)"
        )
    try:
        p = subprocess.run(
            [str(GE_GAME)] + args, capture_output=True, text=True, timeout=timeout
        )
    except subprocess.TimeoutExpired:
        return None, "ge-game timed out after %ss" % timeout
    out = p.stdout.strip()
    err = p.stderr.strip()
    if err and not out:
        return None, err
    return out, err


def call_tool(name, arguments):
    if name == "gengine_new_game":
        return run_cli(["new", str(arguments.get("dir", ""))])
    if name == "gengine_check_game":
        return run_cli(["check", str(arguments.get("dir", ""))])
    if name == "gengine_simulate_game":
        args = ["simulate", str(arguments.get("dir", ""))]
        if "seconds" in arguments:
            args += ["--seconds", str(arguments["seconds"])]
        if arguments.get("input"):
            args += ["--input", str(arguments["input"])]
        return run_cli(args)
    if name == "gengine_pack_game":
        args = ["pack", str(arguments.get("dir", ""))]
        if arguments.get("output"):
            args += ["-o", str(arguments["output"])]
        return run_cli(args)
    if name == "kotha_help":
        try:
            text = KOTHA_DOC.read_text(encoding="utf-8")
        except OSError as e:
            return None, "cannot read docs/KOTHA.md: %s" % e
        return text[:20000], None
    return None, "unknown tool: %s" % name


def send(message):
    sys.stdout.write(json.dumps(message, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def main():
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            send({"jsonrpc": "2.0", "id": None, "error": {"code": -32700, "message": "parse error"}})
            continue

        method = msg.get("method", "")
        msg_id = msg.get("id")
        is_notification = "id" not in msg

        if method == "initialize":
            send({
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "protocolVersion": msg.get("params", {}).get("protocolVersion", PROTOCOL_VERSION),
                    "capabilities": {"tools": {}},
                    "serverInfo": {"name": "g-engine", "version": "0.1.0"},
                },
            })
        elif method in ("notifications/initialized", "notifications/cancelled"):
            pass  # notifications need no reply
        elif method == "ping":
            send({"jsonrpc": "2.0", "id": msg_id, "result": {}})
        elif method == "tools/list":
            send({"jsonrpc": "2.0", "id": msg_id, "result": {"tools": TOOLS}})
        elif method == "tools/call":
            params = msg.get("params", {})
            name = params.get("name", "")
            arguments = params.get("arguments", {}) or {}
            out, err = call_tool(name, arguments)
            if out is None:
                send({"jsonrpc": "2.0", "id": msg_id, "result": {
                    "content": [{"type": "text", "text": err}],
                    "isError": True,
                }})
            else:
                text = out if not err else out + "\n" + err
                send({"jsonrpc": "2.0", "id": msg_id, "result": {
                    "content": [{"type": "text", "text": text}],
                    "isError": False,
                }})
        elif is_notification:
            pass
        else:
            send({"jsonrpc": "2.0", "id": msg_id,
                  "error": {"code": -32601, "message": "method not found: %s" % method}})


if __name__ == "__main__":
    main()
