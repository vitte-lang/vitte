

#!/usr/bin/env python3
"""refresh_fuzz.py

Refresh fuzz-oriented fixture sets for Vitte.

Scope
This script is a specialized orchestrator focused on fuzz inputs and long-run
stress datasets used by:
- lexer/parser fuzzing
- semantic pass fuzzing
- formatter round-trip fuzzing

It runs a subset of the generators (based on manifest.json) with elevated
stress defaults and writes a dedicated report.

Compared to refresh_all.py
- default selection is fuzz-heavy generators
- provides dedicated knobs (iterations, long_ident_size, unicode_mix)
- can generate "campaign" directories for CI fuzz jobs

Outputs
- <OUT>/refresh_fuzz_report.json
- <OUT>/<generator>/... as produced by each generator

Exit codes
- 0: success
- 2: usage error
- 3: IO/internal error
- 4: at least one generator failed

Examples
  python3 refresh_fuzz.py --out ../generated --seed 0
  python3 refresh_fuzz.py --out ../generated --seed 0 --count 200 --stress 4 --emit-md
  python3 refresh_fuzz.py --out ../generated --seed 0 --only tokens,trivia
  python3 refresh_fuzz.py --out ../generated --seed 0 --campaign ../generated/campaigns/fuzz_001

"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple


# -----------------------------------------------------------------------------
# Utilities
# -----------------------------------------------------------------------------


def _now_iso() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime())


def _eprint(*a: object) -> None:
    print(*a, file=sys.stderr)


def _read_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def _write_json(path: Path, obj: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with tmp.open("w", encoding="utf-8", newline="\n") as f:
        json.dump(obj, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    tmp.replace(path)


def _csv_list(s: str) -> List[str]:
    if not s:
        return []
    return [x.strip() for x in s.split(",") if x.strip()]


def _which(cmd: str) -> Optional[str]:
    for p in os.environ.get("PATH", "").split(os.pathsep):
        cand = Path(p) / cmd
        if cand.is_file() and os.access(str(cand), os.X_OK):
            return str(cand)
    return None


def _shell_quote(s: str) -> str:
    if s == "":
        return "''"
    safe = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_./:-"
    if all(ch in safe for ch in s):
        return s
    return "'" + s.replace("'", "'\\''") + "'"


# -----------------------------------------------------------------------------
# Manifest model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Gen:
    name: str
    script: str
    out_subdir: str
    supports_count: bool
    supports_stress: bool
    supports_emit_md: bool
    optional: bool


def load_gens(manifest_path: Path) -> List[Gen]:
    m = _read_json(manifest_path)
    if m.get("schema") != "vitte.bench.generators.manifest.v1":
        raise ValueError(f"unsupported manifest schema: {m.get('schema')}")

    gens: List[Gen] = []
    for g in m.get("generators", []):
        supports = g.get("supports", {})
        gens.append(
            Gen(
                name=str(g.get("name", "")),
                script=str(g.get("script", "")),
                out_subdir=str(g.get("out_subdir", g.get("name", ""))),
                supports_count=bool(supports.get("count", False)),
                supports_stress=bool(supports.get("stress", False)),
                supports_emit_md=bool(supports.get("emit_md", False)),
                optional=bool(supports.get("optional", False)),
            )
        )

    return gens


# -----------------------------------------------------------------------------
# Runner
# -----------------------------------------------------------------------------


@dataclass
class RunResult:
    name: str
    script: str
    out_dir: str
    argv: List[str]
    returncode: int
    elapsed_ms: int


def build_cmd(
    *,
    py: str,
    script_path: Path,
    out_dir: Path,
    seed: str,
    count: int,
    stress: int,
    emit_md: bool,
    quiet: bool,
    verbose: bool,
    gen: Gen,
    extra_args: List[str],
) -> List[str]:
    cmd: List[str] = [py, str(script_path), "--out", str(out_dir), "--seed", str(seed)]

    if gen.supports_count:
        cmd += ["--count", str(int(count))]
    if gen.supports_stress:
        cmd += ["--stress", str(int(stress))]

    if emit_md and gen.supports_emit_md:
        cmd.append("--emit-md")

    if quiet:
        cmd.append("--quiet")
    if verbose:
        cmd.append("--verbose")

    # Extra args are appended last to allow specialized flags.
    cmd += list(extra_args)

    return cmd


def run_one(cmd: List[str], *, cwd: Path, env: Dict[str, str], dry_run: bool) -> Tuple[int, int]:
    t0 = time.perf_counter()

    if dry_run:
        rc = 0
    else:
        p = subprocess.run(cmd, cwd=str(cwd), env=env)
        rc = int(p.returncode)

    dt_ms = int((time.perf_counter() - t0) * 1000.0)
    return rc, dt_ms


# -----------------------------------------------------------------------------
# Campaign
# -----------------------------------------------------------------------------


def write_campaign_stub(campaign_dir: Path, *, seed: str, count: int, stress: int, only: List[str]) -> None:
    """Write a small campaign metadata file so CI fuzz jobs can discover intent."""
    campaign_dir.mkdir(parents=True, exist_ok=True)
    _write_json(
        campaign_dir / "campaign.json",
        {
            "schema": "vitte.bench.generators.fuzz_campaign.v1",
            "version": 1,
            "created_at": _now_iso(),
            "seed": seed,
            "count": int(count),
            "stress": int(stress),
            "only": list(only),
        },
    )


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="refresh_fuzz.py")

    ap.add_argument("--out", required=True, help="Output root directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")

    # fuzz-leaning defaults
    ap.add_argument("--count", type=int, default=200, help="Random case count")
    ap.add_argument("--stress", type=int, default=3, help="Stress level")

    ap.add_argument("--emit-md", action="store_true")

    ap.add_argument("--only", default="", help="Comma-separated generator names to run")
    ap.add_argument("--skip", default="", help="Comma-separated generator names to skip")

    ap.add_argument("--python", default=sys.executable, help="Python interpreter path")
    ap.add_argument("--manifest", default="manifest.json", help="Manifest JSON path")

    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--verbose", action="store_true")

    ap.add_argument("--keep-going", action="store_true")
    ap.add_argument("--dry-run", action="store_true")

    # campaign output: if set, all outputs go there
    ap.add_argument("--campaign", default="", help="If set, write a fuzz campaign directory")

    # extra args passed to all generators (advanced)
    ap.add_argument("--extra", default="", help="Extra args appended to each generator invocation")

    return ap.parse_args(list(argv))


def main(argv: Optional[List[str]] = None) -> int:
    ns = parse_args(argv if argv is not None else sys.argv[1:])

    if ns.count < 0:
        _eprint("error: --count must be >= 0")
        return 2
    if ns.stress < 0:
        _eprint("error: --stress must be >= 0")
        return 2

    here = Path(__file__).resolve().parent
    manifest_path = (here / ns.manifest).resolve()

    try:
        gens = load_gens(manifest_path)
    except Exception as e:
        _eprint(f"error: failed to load manifest: {e}")
        return 3

    # Default fuzz subset
    default_only = ["tokens", "trivia", "unicode", "unicode_idents"]

    only = _csv_list(str(ns.only))
    skip = set(_csv_list(str(ns.skip)))

    if not only:
        only = list(default_only)

    # Resolve list to actual gens
    by_name: Dict[str, Gen] = {g.name: g for g in gens}
    selected: List[Gen] = []
    for name in only:
        g = by_name.get(name)
        if g is None:
            _eprint(f"error: unknown generator: {name}")
            return 2
        if name in skip:
            continue

        script_path = here / g.script
        if not script_path.is_file():
            if g.optional:
                continue
            _eprint(f"error: required generator script missing: {g.script}")
            return 3

        selected.append(g)

    if not selected:
        _eprint("error: no generators selected")
        return 2

    py = str(ns.python)
    if not Path(py).exists():
        found = _which(py)
        if found is None:
            _eprint(f"error: python not found: {py}")
            return 3
        py = found

    # Campaign redirects out_root
    out_root = Path(ns.out).expanduser().resolve()
    campaign_dir: Optional[Path] = None
    if ns.campaign:
        campaign_dir = Path(ns.campaign).expanduser().resolve()
        out_root = campaign_dir

    out_root.mkdir(parents=True, exist_ok=True)

    env = dict(os.environ)

    extra_args = []
    if ns.extra:
        # This is intentionally naive splitting to keep deterministic.
        # Users needing complex quoting should run generators directly.
        extra_args = [x for x in str(ns.extra).split(" ") if x]

    results: List[RunResult] = []
    ok = True

    for g in selected:
        script_path = (here / g.script).resolve()
        out_dir = out_root / g.out_subdir

        cmd = build_cmd(
            py=py,
            script_path=script_path,
            out_dir=out_dir,
            seed=str(ns.seed),
            count=int(ns.count),
            stress=int(ns.stress),
            emit_md=bool(ns.emit_md),
            quiet=bool(ns.quiet),
            verbose=bool(ns.verbose),
            gen=g,
            extra_args=extra_args,
        )

        if ns.dry_run:
            print(" ".join(_shell_quote(a) for a in cmd))

        rc, ms = run_one(cmd, cwd=here, env=env, dry_run=bool(ns.dry_run))

        results.append(
            RunResult(
                name=g.name,
                script=g.script,
                out_dir=str(out_dir),
                argv=cmd,
                returncode=rc,
                elapsed_ms=ms,
            )
        )

        if rc != 0:
            ok = False
            if not ns.keep_going:
                break

    # Campaign metadata (if requested)
    if campaign_dir is not None:
        try:
            write_campaign_stub(
                campaign_dir,
                seed=str(ns.seed),
                count=int(ns.count),
                stress=int(ns.stress),
                only=[g.name for g in selected],
            )
        except Exception as e:
            _eprint(f"error: failed to write campaign metadata: {e}")
            return 3

    report = {
        "schema": "vitte.bench.generators.refresh_fuzz_report.v1",
        "version": 1,
        "generated_at": _now_iso(),
        "cwd": str(here),
        "python": py,
        "out": str(out_root),
        "args": {
            "seed": str(ns.seed),
            "count": int(ns.count),
            "stress": int(ns.stress),
            "emit_md": bool(ns.emit_md),
            "only": [g.name for g in selected],
            "skip": list(sorted(skip)),
            "quiet": bool(ns.quiet),
            "verbose": bool(ns.verbose),
            "keep_going": bool(ns.keep_going),
            "dry_run": bool(ns.dry_run),
            "campaign": str(ns.campaign) if ns.campaign else "",
            "extra": str(ns.extra),
        },
        "results": [
            {
                "name": r.name,
                "script": r.script,
                "out_dir": r.out_dir,
                "returncode": int(r.returncode),
                "elapsed_ms": int(r.elapsed_ms),
                "argv": r.argv,
            }
            for r in results
        ],
        "ok": bool(ok),
    }

    try:
        _write_json(out_root / "refresh_fuzz_report.json", report)
    except Exception as e:
        _eprint(f"error: failed to write report: {e}")
        return 3

    if ok:
        return 0
    return 4


if __name__ == "__main__":
    raise SystemExit(main())