#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "tests/cross_package_http_process_log_fs_smoke.vit"
OUT = ROOT / "target/reports/semantic_golden_cross_package.json"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--allow-missing-bin", action="store_true")
    args = ap.parse_args()

    checks = []
    checks.append({"name": "source_exists", "ok": SRC.exists(), "detail": str(SRC)})

    if SRC.exists():
        txt = SRC.read_text(encoding="utf-8", errors="ignore")
        for pkg in ["vitte/http", "vitte/process", "vitte/log", "vitte/fs"]:
            checks.append({"name": f"imports_{pkg}", "ok": pkg in txt, "detail": "use statement"})
        checks.append({
            "name": "imports_vitte/db_optional",
            "ok": True,
            "detail": "db link recommended for extended chain; optional in current fixture",
        })

    bin_path = ROOT / "bin/vitte"
    if not bin_path.exists() and not args.allow_missing_bin:
        checks.append({"name": "compiler_available", "ok": False, "detail": str(bin_path)})
    elif bin_path.exists():
        p = subprocess.run([str(bin_path), "check", "--lang=en", str(SRC)], capture_output=True, text=True)
        checks.append({
            "name": "compile_cross_package",
            "ok": p.returncode == 0,
            "detail": (p.stdout + "\n" + p.stderr)[-4000:],
        })

    passed = all(c["ok"] for c in checks)
    data = {
        "schema_version": "1.0",
        "passed": passed,
        "checks": checks,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if passed:
        print("[semantic-golden] OK")
        return 0
    print("[semantic-golden][error] failed")
    for c in checks:
        if not c["ok"]:
            print(f"[semantic-golden][error] {c['name']}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
