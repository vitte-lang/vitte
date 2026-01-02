

#!/usr/bin/env python3
"""refresh_all.py

Orchestrate running all Vitte bench data generators.

Why this exists
- CI and developers often want a single entrypoint to refresh the entire
  generated fixture set in a deterministic way.
- The generator suite is modular (multiple gen_*.py scripts). This tool
  standardizes invocation, logging, ordering, and writes a unified report.

Behavior
- Discovers generators via `manifest.json` in this directory.
- Runs generators sequentially (stable order) unless filtered.
- Writes a machine-readable report: <OUT>/refresh_report.json

Exit codes
- 0: all requested generators succeeded
- 2: usage / invalid args
- 3: IO/internal error
- 4: at least one generator failed

Examples
  python3 refresh_all.py --out ../generated --seed 0
  python3 refresh_all.py --out ../generated --seed 0 --count 50 --stress 2 --emit-md
  python3 refresh_all.py --out ../generated --seed 0 --only tokens,trivia
  python3 refresh_all.py --out ../generated --seed 0 --skip unicode

Stats passthrough:
  python3 refresh_all.py --out ../generated --seed 0 \
    --stats-inputs ../generated/parse ../generated/sema --stats-include-ext vitte,muf
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
    # Keep ISO-ish without timezone ambiguity; consumers can attach timezone.
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
    # Minimal PATH lookup.
    for p in os.environ.get("PATH", "").split(os.pathsep):
        cand = Path(p) / cmd
        if cand.is_file() and os.access(str(cand), os.X_OK):
            return str(cand)
    return None


# -----------------------------------------------------------------------------
# Manifest model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Gen:
    name: str
    script: str
    out_subdir: str
    contract_name: str
    contract_version: int
    supports_count: bool
    supports_stress: bool
    supports_emit_md: bool
    optional: bool


def load_manifest(manifest_path: Path) -> Tuple[Dict[str, Any], List[Gen]]:
    m = _read_json(manifest_path)

    if m.get("schema") != "vitte.bench.generators.manifest.v1":
        raise ValueError(f"unsupported manifest schema: {m.get('schema')}")

    gens: List[Gen] = []
    for g in m.get("generators", []):
        name = str(g.get("name", ""))
        script = str(g.get("script", ""))
        out_subdir = str(g.get("out_subdir", name))
        contract = g.get("contract", {})
        supports = g.get("supports", {})

        gens.append(
            Gen(
                name=name,
                script=script,
                out_subdir=out_subdir,
                contract_name=str(contract.get("name", "")),
                contract_version=int(contract.get("version", 0)),
                supports_count=bool(supports.get("count", False)),
                supports_stress=bool(supports.get("stress", False)),
                supports_emit_md=bool(supports.get("emit_md", False)),
                optional=bool(supports.get("optional", False)),
            )
        )

    # Stable ordering: as declared in manifest.
    return m, gens


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
    stats_inputs: Optional[List[str]],
    stats_include_ext: str,
    stats_max_files: int,
    stats_top: int,
) -> List[str]:
    cmd: List[str] = [py, str(script_path), "--out", str(out_dir), "--seed", str(seed)]

    if gen.name == "stats":
        if stats_inputs is not None and len(stats_inputs) > 0:
            cmd += ["--inputs", *stats_inputs]
        # include-ext can be "" (meaning default behavior)
        if stats_include_ext:
            cmd += ["--include-ext", stats_include_ext]
        cmd += ["--max-files", str(int(stats_max_files))]
        cmd += ["--top", str(int(stats_top))]

    else:
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
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="refresh_all.py")

    ap.add_argument("--out", required=True, help="Output root directory (e.g. ../generated)")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--count", type=int, default=0, help="Random case count for supporting generators")
    ap.add_argument("--stress", type=int, default=0, help="Stress level for supporting generators")
    ap.add_argument("--emit-md", action="store_true", help="Emit INDEX.md where supported")

    ap.add_argument("--only", default="", help="Comma-separated generator names to run")
    ap.add_argument("--skip", default="", help="Comma-separated generator names to skip")

    ap.add_argument("--python", default=sys.executable, help="Python interpreter path")
    ap.add_argument("--manifest", default="manifest.json", help="Manifest JSON path")

    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--verbose", action="store_true")

    ap.add_argument("--keep-going", action="store_true", help="Do not stop on first failure")
    ap.add_argument("--dry-run", action="store_true", help="Print commands but do not execute")

    # gen_stats passthrough
    ap.add_argument("--stats-inputs", nargs="*", default=None, help="Roots to scan (gen_stats.py)")
    ap.add_argument("--stats-include-ext", default="", help="Comma-separated allowlist (gen_stats.py)")
    ap.add_argument("--stats-max-files", type=int, default=0, help="Max files (0=unlimited) (gen_stats.py)")
    ap.add_argument("--stats-top", type=int, default=50, help="Top files count (gen_stats.py)")

    return ap.parse_args(list(argv))


def main(argv: Optional[List[str]] = None) -> int:
    ns = parse_args(argv if argv is not None else sys.argv[1:])

    if ns.count < 0:
        _eprint("error: --count must be >= 0")
        return 2
    if ns.stress < 0:
        _eprint("error: --stress must be >= 0")
        return 2
    if ns.stats_max_files < 0:
        _eprint("error: --stats-max-files must be >= 0")
        return 2
    if ns.stats_top <= 0:
        _eprint("error: --stats-top must be > 0")
        return 2

    here = Path(__file__).resolve().parent
    manifest_path = (here / ns.manifest).resolve()

    try:
        manifest, gens = load_manifest(manifest_path)
    except Exception as e:
        _eprint(f"error: failed to load manifest: {e}")
        return 3

    only = set(_csv_list(str(ns.only)))
    skip = set(_csv_list(str(ns.skip)))

    # Filter
    selected: List[Gen] = []
    for g in gens:
        if only and g.name not in only:
            continue
        if g.name in skip:
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
        # If it's a bare name, attempt PATH lookup.
        found = _which(py)
        if found is None:
            _eprint(f"error: python not found: {py}")
            return 3
        py = found

    out_root = Path(ns.out).expanduser().resolve()
    out_root.mkdir(parents=True, exist_ok=True)

    env = dict(os.environ)

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
            stats_inputs=list(ns.stats_inputs) if ns.stats_inputs is not None else None,
            stats_include_ext=str(ns.stats_include_ext),
            stats_max_files=int(ns.stats_max_files),
            stats_top=int(ns.stats_top),
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

    report = {
        "schema": "vitte.bench.generators.refresh_report.v1",
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
            "only": list(sorted(only)),
            "skip": list(sorted(skip)),
            "quiet": bool(ns.quiet),
            "verbose": bool(ns.verbose),
            "keep_going": bool(ns.keep_going),
            "dry_run": bool(ns.dry_run),
            "stats_inputs": list(ns.stats_inputs) if ns.stats_inputs is not None else None,
            "stats_include_ext": str(ns.stats_include_ext),
            "stats_max_files": int(ns.stats_max_files),
            "stats_top": int(ns.stats_top),
        },
        "manifest": {
            "path": str(manifest_path),
            "schema": manifest.get("schema"),
            "version": manifest.get("version"),
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
        _write_json(out_root / "refresh_report.json", report)
    except Exception as e:
        _eprint(f"error: failed to write report: {e}")
        return 3

    if ok:
        return 0
    return 4


def _shell_quote(s: str) -> str:
    # Minimal POSIX shell quoting for display.
    if s == "":
        return "''"
    if re_safe(s):
        return s
    return "'" + s.replace("'", "'\\''") + "'"


def re_safe(s: str) -> bool:
    # avoid importing re just for a tiny predicate; keep stable.
    safe = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_./:-"
    return all(ch in safe for ch in s)


if __name__ == "__main__":
    raise SystemExit(main())