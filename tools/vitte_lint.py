#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

CHECKS = [
    ("package-layout", ["tools/lint_package_layout.py"]),
    ("legacy-imports", ["tools/lint_legacy_import_paths.py", "--max-entries=5"]),
    ("core-forbidden-syntax", ["python3", "tools/lint_core_forbidden_syntax.py"]),
    ("diagnostic-style", ["python3", "tools/check_suggestion_quality.py"]),
]


def run_check(name: str, cmd: list[str]) -> tuple[str, bool, str]:
    completed = subprocess.run(
        cmd,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return name, completed.returncode == 0, completed.stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(description="Official Vitte linter")
    parser.add_argument("--list", action="store_true", help="list official lint checks")
    parser.add_argument("--check", action="store_true", help="run the official lint suite")
    args = parser.parse_args()

    if args.list:
        for name, _ in CHECKS:
            print(name)
        return 0

    failures: list[str] = []
    for name, cmd in CHECKS:
        check_name, ok, output = run_check(name, cmd)
        if ok:
            print(f"[vitte-lint] ok {check_name}")
        else:
            print(f"[vitte-lint][error] {check_name}")
            if output:
                print(output)
            failures.append(check_name)

    if failures:
        print(f"[vitte-lint][error] failed={','.join(failures)}")
        return 1
    print(f"[vitte-lint] OK checks={len(CHECKS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
