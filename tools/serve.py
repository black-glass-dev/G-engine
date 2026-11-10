#!/usr/bin/env python3
"""Serves the web player (the web/ folder) on this computer and on the local network.

    python3 tools/serve.py            # http://localhost:8080
    python3 tools/serve.py 9000       # another port

Open the address on a phone connected to the same Wi-Fi to try the game there.
"""
import functools
import http.server
import os
import socket
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "web")


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
        ".mjs": "text/javascript",
        ".woff2": "font/woff2",
    }

    def end_headers(self):
        # Always serve the newest build while developing.
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, fmt, *args):
        if os.environ.get("GE_SERVE_QUIET") != "1":
            super().log_message(fmt, *args)


def local_address():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("10.255.255.255", 1))
            return s.getsockname()[0]
    except OSError:
        return None


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    if not os.path.exists(os.path.join(ROOT, "g-engine.wasm")):
        print("note: web/g-engine.wasm is missing - run tools/build_web.sh first", flush=True)
    handler = functools.partial(Handler, directory=ROOT)
    http.server.ThreadingHTTPServer.allow_reuse_address = True
    with http.server.ThreadingHTTPServer(("0.0.0.0", port), handler) as server:
        print(f"G-Engine web player:  http://localhost:{port}", flush=True)
        address = local_address()
        if address:
            print(f"on the same Wi-Fi:    http://{address}:{port}", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
