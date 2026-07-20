#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "bin" / "vitte"
NEGATIVE_DIR = ROOT / "tests" / "diagnostics" / "negative"
EXPECT_SUFFIXES = (".stderr", ".snap")
sys.path.insert(0, str(ROOT / "tools"))

from diagnostic_frontend import analyze  # noqa: E402


def normalize(text: str) -> str:
    return re.sub(r"\x1b\[[0-9;]*m", "", text).replace("\r\n", "\n")


def parse_snapshot(path: Path) -> list[dict[str, int | str]]:
    expected: list[dict[str, int | str]] = []
    current: dict[str, int | str] | None = None
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if stripped.startswith("- code:"):
            current = {"code": stripped.split(":", 1)[1].strip()}
            expected.append(current)
        elif current is not None and stripped.startswith(("title:", "line:", "column:")):
            key, value = stripped.split(":", 1)
            value = value.strip()
            current[key] = int(value) if key in {"line", "column"} else value
    return expected


def check_snapshot(fixture: Path, expect: Path) -> None:
    expected = parse_snapshot(expect)
    if not expected:
        raise SystemExit(f"[diagnostics-negative][error] empty snapshot: {expect}")
    diagnostics = analyze(fixture.read_text(encoding="utf-8"), fixture.name)
    actual = [
        {
            "code": diagnostic["code"],
            "title": diagnostic["message"],
            "line": diagnostic["primary_span"]["start"]["line"],
            "column": diagnostic["primary_span"]["start"]["column"],
        }
        for diagnostic in diagnostics
    ]
    if actual != expected:
        raise SystemExit(f"[diagnostics-negative][error] {fixture}: expected {expected}, got {actual}")


def main() -> int:
    if not BIN.exists():
        raise SystemExit(f"[diagnostics-negative][error] missing compiler binary: {BIN}")
    fixtures = sorted(NEGATIVE_DIR.glob("*/*.vit"))
    if not fixtures:
        raise SystemExit(f"[diagnostics-negative][error] no fixtures in {NEGATIVE_DIR}")
    for fixture in fixtures:
        expectations = [fixture.with_suffix(suffix) for suffix in EXPECT_SUFFIXES if fixture.with_suffix(suffix).exists()]
        if not expectations:
            expected = ", ".join(str(fixture.with_suffix(suffix)) for suffix in EXPECT_SUFFIXES)
            raise SystemExit(f"[diagnostics-negative][error] missing expectation: {expected}")
        if len(expectations) != 1:
            raise SystemExit(f"[diagnostics-negative][error] multiple expectations for {fixture}: {expectations}")
        expect = expectations[0]
        if expect.suffix == ".snap":
            check_snapshot(fixture, expect)
            continue
        proc = subprocess.run(
            [str(BIN), "check", "--lang=en", str(fixture.relative_to(ROOT))],
            cwd=str(ROOT),
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        if proc.returncode == 0:
            raise SystemExit(f"[diagnostics-negative][error] {fixture}: expected diagnostic failure")
        output = normalize(proc.stdout)
        for needle in normalize(expect.read_text(encoding="utf-8")).splitlines():
            if needle and needle not in output:
                raise SystemExit(f"[diagnostics-negative][error] {fixture}: missing {needle!r}\n{output}")
    print(f"[diagnostics-negative] checked {len(fixtures)} fixture(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
