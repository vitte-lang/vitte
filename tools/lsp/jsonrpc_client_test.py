#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
VITTE = ROOT / "bin" / "vitte"
FIXTURES = [
    ROOT / "tests" / "lsp" / "fixtures" / "vscode_references.json",
    ROOT / "tests" / "lsp" / "fixtures" / "neovim_workspace_symbols.json",
    ROOT / "tests" / "lsp" / "fixtures" / "vscode_code_action.json",
]


def framed(payload: dict[str, Any]) -> bytes:
    body = json.dumps(payload, separators=(",", ":"), sort_keys=True).encode("utf-8")
    return b"Content-Length: " + str(len(body)).encode("ascii") + b"\r\n\r\n" + body


def parse_framed(data: bytes) -> dict[str, Any]:
    header, sep, body = data.partition(b"\r\n\r\n")
    if not sep:
        raise AssertionError(f"missing JSON-RPC header: {data!r}")
    length = None
    for line in header.decode("ascii").splitlines():
        if line.lower().startswith("content-length:"):
            length = int(line.split(":", 1)[1].strip())
    if length is None:
        raise AssertionError("missing Content-Length")
    return json.loads(body[:length].decode("utf-8"))


def run_fixture(path: Path) -> dict[str, Any]:
    request = json.loads(path.read_text(encoding="utf-8"))
    completed = subprocess.run(
        [str(VITTE), "lsp", "--stdio", "--once"],
        input=framed(request),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=ROOT,
        check=False,
    )
    if completed.returncode != 0:
        raise AssertionError(completed.stderr.decode("utf-8", errors="replace"))
    response = parse_framed(completed.stdout)
    assert response["jsonrpc"] == "2.0"
    assert response["id"] == request["id"]
    assert "result" in response
    return response


def main() -> int:
    responses = {fixture.name: run_fixture(fixture) for fixture in FIXTURES}
    assert len(responses["vscode_references.json"]["result"]) >= 3
    assert responses["neovim_workspace_symbols.json"]["result"][0]["name"] == "main_proc"
    assert responses["vscode_code_action.json"]["result"][0]["kind"] == "quickfix"
    print("[lsp-jsonrpc] checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
