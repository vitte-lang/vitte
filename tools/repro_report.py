#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import platform
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "target/reports/repro.json"


def run(cmd: list[str]) -> str:
    try:
        p = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, check=False)
        if p.returncode != 0:
            return ""
        return p.stdout.strip()
    except Exception:
        return ""


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    if not path.exists():
        return ""
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    data = {
        "schema_version": "1.0",
        "platform": platform.platform(),
        "python": platform.python_version(),
        "compiler_cxx": run(["c++", "--version"]).splitlines()[:1],
        "git_commit": run(["git", "rev-parse", "HEAD"]),
        "git_status_short": run(["git", "status", "--short"]),
        "env": {
            "CC": os.environ.get("CC", ""),
            "CXX": os.environ.get("CXX", ""),
            "CFLAGS": os.environ.get("CFLAGS", ""),
            "CXXFLAGS": os.environ.get("CXXFLAGS", ""),
            "LDFLAGS": os.environ.get("LDFLAGS", ""),
        },
        "hashes": {
            "Makefile": sha256_file(ROOT / "Makefile"),
            "bin/vitte": sha256_file(ROOT / "bin/vitte"),
            "plugins/vitte_analyzer_pack.cpp": sha256_file(ROOT / "plugins/vitte_analyzer_pack.cpp"),
        },
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[repro-report] wrote {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
