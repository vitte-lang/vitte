#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def check(path: Path, tag: str) -> int:
    if not path.exists():
        print(f"[{tag}][error] missing snapshot: {path}")
        return 1
    if path.read_text(encoding="utf-8").strip():
        print(f"[{tag}][error] {path}: must stay empty")
        return 1
    print(f"[{tag}] OK")
    return 0


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    a = check(repo / "tests/modules/contracts/http/http.exports.internal", "http-no-internal-exports")
    b = check(repo / "tests/modules/contracts/http_client/http_client.exports.internal", "http-client-no-internal-exports")
    return 1 if (a or b) else 0


if __name__ == "__main__":
    raise SystemExit(main())
