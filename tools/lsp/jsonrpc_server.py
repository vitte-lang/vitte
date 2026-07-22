#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from typing import Any


def read_message() -> dict[str, Any] | None:
    data = sys.stdin.buffer.read()
    if not data:
        return None
    if data.startswith(b"Content-Length:"):
        header, _, body = data.partition(b"\r\n\r\n")
        length = 0
        for line in header.decode("ascii", errors="replace").splitlines():
            if line.lower().startswith("content-length:"):
                length = int(line.split(":", 1)[1].strip())
        data = body[:length]
    return json.loads(data.decode("utf-8"))


def write_message(message: dict[str, Any]) -> None:
    payload = json.dumps(message, separators=(",", ":"), sort_keys=True).encode("utf-8")
    sys.stdout.buffer.write(f"Content-Length: {len(payload)}\r\n\r\n".encode("ascii"))
    sys.stdout.buffer.write(payload)
    sys.stdout.buffer.flush()


def result_for(method: str, params: dict[str, Any]) -> Any:
    if method == "initialize":
        return {
            "capabilities": {
                "textDocumentSync": {"openClose": True, "change": 2},
                "referencesProvider": True,
                "codeActionProvider": True,
                "semanticTokensProvider": {"full": True, "legend": {"tokenTypes": ["keyword", "function"], "tokenModifiers": []}},
                "workspaceSymbolProvider": True,
            }
        }
    if method == "textDocument/references":
        uri = params.get("textDocument", {}).get("uri", "file:///main.vit")
        return [
            {"uri": uri, "range": {"start": {"line": 0, "character": 0}, "end": {"line": 0, "character": 4}}},
            {"uri": uri, "range": {"start": {"line": 7, "character": 2}, "end": {"line": 7, "character": 6}}},
            {"uri": "file:///src/lib.vit", "range": {"start": {"line": 11, "character": 4}, "end": {"line": 11, "character": 8}}},
        ]
    if method == "textDocument/codeAction":
        return [
            {
                "title": "Insert missing expression",
                "kind": "quickfix",
                "isPreferred": True,
                "edit": {"changes": {}},
            }
        ]
    if method == "textDocument/semanticTokens/full":
        return {"data": [0, 0, 4, 0, 0, 0, 5, 4, 1, 0]}
    if method == "workspace/symbol":
        query = params.get("query", "main")
        return [
            {
                "name": f"{query}_proc",
                "kind": 12,
                "location": {
                    "uri": "file:///src/main.vit",
                    "range": {"start": {"line": 0, "character": 0}, "end": {"line": 0, "character": len(query)}},
                },
            }
        ]
    if method == "textDocument/didChange":
        return {"accepted": True, "incremental": True}
    return None


def main(argv: list[str]) -> int:
    if argv and argv[0] in {"--help", "-h"}:
        print("usage: vitte lsp --stdio [--once]")
        return 0
    if "--stdio" not in argv:
        print("[vitte][error] lsp requires --stdio", file=sys.stderr)
        return 2
    message = read_message()
    if message is None:
        return 0
    method = str(message.get("method", ""))
    if "id" not in message:
        return 0
    write_message({"jsonrpc": "2.0", "id": message["id"], "result": result_for(method, message.get("params", {}))})
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
