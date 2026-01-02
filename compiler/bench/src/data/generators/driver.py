#!/usr/bin/env python3
"""driver.py

Unified driver for Vitte bench/data generators.

This is the entrypoint used by CI and local workflows to:
- run corpus/fixture generators
- normalize outputs
- emit manifests
- diff manifests
- sample corpora for debugging
- validate JSON fixtures against schemas

The driver is intentionally stdlib-first and composes existing tools.

Typical usage:
  python3 driver.py generate --out ../../data/generated --seed 0
  python3 driver.py normalize-lf --root ../../data/generated
  python3 driver.py manifest --root ../../data/generated --out manifest.json
  python3 driver.py manifest-diff old.json new.json --format md --out report.md
  python3 driver.py sample --in ../../data/generated --out ../../data/sampled --count 200 --seed 123
  python3 driver.py validate-schema --schema schemas/case.schema.json --in ../../data/cases

Exit codes:
- 0 success
- 1 differences found / validation failures (subcommand specific)
- 2 usage/input error
- 3 IO/internal error
"""

from __future__ import annotations

import argparse
import os
import runpy
import sys
from pathlib import Path
from typing import List, Optional, Sequence


# -----------------------------------------------------------------------------
# Path bootstrap
# -----------------------------------------------------------------------------


def _script_dir() -> Path:
    return Path(__file__).resolve().parent


def _add_pkg_roots() -> None:
    """Ensure vitte_benchgen and tools are importable when run from repo."""

    root = _script_dir()

    # Add `.../generators` and `.../generators/vitte_benchgen` parent.
    if str(root) not in sys.path:
        sys.path.insert(0, str(root))

    pkg = root / "vitte_benchgen"
    if pkg.exists():
        if str(root) not in sys.path:
            sys.path.insert(0, str(root))


_add_pkg_roots()


# -----------------------------------------------------------------------------
# Small helpers
# -----------------------------------------------------------------------------


def _eprint(*a: object) -> None:
    print(*a, file=sys.stderr)


def _run_tool_module(path: Path, argv: List[str]) -> int:
    """Run a sibling tool script as if invoked as __main__."""

    if not path.exists() or not path.is_file():
        _eprint(f"error: tool not found: {path}")
        return 2

    old_argv = sys.argv
    try:
        sys.argv = [path.name] + argv
        # run as script
        runpy.run_path(str(path), run_name="__main__")
        return 0
    except SystemExit as e:
        if isinstance(e.code, int):
            return e.code
        return 2
    except Exception as e:
        _eprint(f"error: tool failed: {path.name}: {e}")
        return 3
    finally:
        sys.argv = old_argv


# -----------------------------------------------------------------------------
# Commands
# -----------------------------------------------------------------------------


def cmd_generate(ns: argparse.Namespace) -> int:
    """Run generator pipeline.

    This driver intentionally keeps generator logic lightweight.
    The actual generator implementation is expected to live in `vitte_benchgen`.
    """

    out_dir = Path(ns.out).expanduser().resolve()
    seed = str(ns.seed)

    try:
        from vitte_benchgen import BenchGen, GeneratorConfig, load_config
        from vitte_benchgen.util import Logger
    except Exception as e:
        _eprint(f"error: cannot import vitte_benchgen: {e}")
        return 3

    log = Logger(level="debug" if ns.verbose else "info", quiet=bool(ns.quiet))

    cfg_path = Path(ns.config).expanduser().resolve() if ns.config else None
    cfg: Optional[GeneratorConfig] = None

    if cfg_path is not None:
        try:
            cfg = load_config(cfg_path)
        except Exception as e:
            _eprint(f"error: failed to load config: {cfg_path}: {e}")
            return 2

    try:
        gen = BenchGen(out_dir=out_dir, seed=seed, logger=log, config=cfg)
        gen.run(all_targets=bool(ns.all))
    except TypeError:
        # If the local package skeleton doesn't implement BenchGen yet,
        # provide a clear message instead of crashing.
        _eprint("error: BenchGen API not implemented in vitte_benchgen (missing benchgen.py)")
        return 3
    except Exception as e:
        _eprint(f"error: generation failed: {e}")
        return 3

    return 0


def cmd_manifest(ns: argparse.Namespace) -> int:
    root = Path(ns.root).expanduser().resolve()
    out = Path(ns.out).expanduser().resolve()

    try:
        from vitte_benchgen.util import Logger
        from vitte_benchgen.writer import TreeWriter
    except Exception as e:
        _eprint(f"error: cannot import vitte_benchgen: {e}")
        return 3

    if not root.exists() or not root.is_dir():
        _eprint(f"error: --root not a directory: {root}")
        return 2

    log = Logger(level="debug" if ns.verbose else "info", quiet=bool(ns.quiet))

    tw = TreeWriter(root, logger=log)
    man = tw.build_manifest(hash_files=not ns.no_hash)

    # Stable JSON output using util.write_json
    try:
        from vitte_benchgen.util import write_json

        write_json(out, man)
    except Exception as e:
        _eprint(f"error: writing manifest failed: {e}")
        return 3

    if not ns.quiet:
        print(f"manifest: root={root} out={out} files={len(man.get('files', []))}")

    return 0


def cmd_manifest_diff(ns: argparse.Namespace) -> int:
    tool = _script_dir() / "tools" / "manifest_diff.py"
    argv = [ns.old, ns.new]
    argv += ["--format", ns.format]
    if ns.out:
        argv += ["--out", ns.out]
    if ns.ignore:
        for it in ns.ignore:
            argv += ["--ignore", it]
    if ns.only:
        for it in ns.only:
            argv += ["--only", it]
    if ns.text_diff:
        argv += ["--text-diff"]
        if ns.base_old:
            argv += ["--base-old", ns.base_old]
        if ns.base_new:
            argv += ["--base-new", ns.base_new]
    if ns.text_max_bytes:
        argv += ["--text-max-bytes", str(ns.text_max_bytes)]
    if ns.diff_context is not None:
        argv += ["--diff-context", str(ns.diff_context)]
    if ns.quiet:
        argv += ["--quiet"]

    return _run_tool_module(tool, argv)


def cmd_normalize_lf(ns: argparse.Namespace) -> int:
    tool = _script_dir() / "tools" / "normalize_lf.py"
    argv = ["--root", ns.root]
    if ns.include:
        for it in ns.include:
            argv += ["--include", it]
    if ns.exclude:
        for it in ns.exclude:
            argv += ["--exclude", it]
    if ns.ext:
        for it in ns.ext:
            argv += ["--ext", it]
    if ns.encoding:
        argv += ["--encoding", ns.encoding]
    if ns.max_bytes:
        argv += ["--max-bytes", str(ns.max_bytes)]
    if ns.dry_run:
        argv += ["--dry-run"]
    if ns.verbose:
        argv += ["--verbose"]
    if ns.quiet:
        argv += ["--quiet"]

    return _run_tool_module(tool, argv)


def cmd_sample(ns: argparse.Namespace) -> int:
    tool = _script_dir() / "tools" / "sample_corpus.py"
    argv = ["--in", ns.in_dir, "--out", ns.out_dir]

    if ns.count:
        argv += ["--count", str(ns.count)]
    if ns.bytes:
        argv += ["--bytes", str(ns.bytes)]
    if ns.pct:
        argv += ["--pct", str(ns.pct)]

    argv += ["--seed", str(ns.seed)]

    if ns.include:
        for it in ns.include:
            argv += ["--include", it]
    if ns.exclude:
        for it in ns.exclude:
            argv += ["--exclude", it]
    if ns.ext:
        for it in ns.ext:
            argv += ["--ext", it]

    if ns.manifest:
        argv += ["--manifest", ns.manifest]

    if ns.clean:
        argv += ["--clean"]
    if ns.dry_run:
        argv += ["--dry-run"]
    if ns.verbose:
        argv += ["--verbose"]
    if ns.quiet:
        argv += ["--quiet"]

    return _run_tool_module(tool, argv)


def cmd_validate_schema(ns: argparse.Namespace) -> int:
    tool = _script_dir() / "tools" / "validate_json_schema.py"
    argv: List[str] = ["--schema", ns.schema]

    if ns.in_dir:
        argv += ["--in", ns.in_dir]

    if ns.files:
        argv += ["--files"] + list(ns.files)

    if ns.include:
        for it in ns.include:
            argv += ["--include", it]
    if ns.exclude:
        for it in ns.exclude:
            argv += ["--exclude", it]

    if ns.report:
        argv += ["--report", ns.report]
    if ns.verbose:
        argv += ["--verbose"]
    if ns.quiet:
        argv += ["--quiet"]
    if ns.max_issues is not None:
        argv += ["--max-issues", str(ns.max_issues)]

    return _run_tool_module(tool, argv)


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="driver.py")

    ap.add_argument("--quiet", action="store_true", help="Reduce output")
    ap.add_argument("--verbose", action="store_true", help="Increase output")

    sub = ap.add_subparsers(dest="cmd", required=True)

    # generate
    g = sub.add_parser("generate", help="Run bench/data generation pipeline")
    g.add_argument("--out", required=True, help="Output directory")
    g.add_argument("--seed", default="0", help="Deterministic seed")
    g.add_argument("--config", default="", help="Optional config file (json/yaml depending on implementation)")
    g.add_argument("--all", action="store_true", help="Generate all targets (if supported)")
    g.set_defaults(_fn=cmd_generate)

    # manifest
    m = sub.add_parser("manifest", help="Build manifest.json for a directory")
    m.add_argument("--root", required=True, help="Root directory")
    m.add_argument("--out", required=True, help="Manifest output path")
    m.add_argument("--no-hash", action="store_true", help="Skip sha256")
    m.set_defaults(_fn=cmd_manifest)

    # manifest-diff
    md = sub.add_parser("manifest-diff", help="Diff two manifest JSON files")
    md.add_argument("old", help="Old manifest")
    md.add_argument("new", help="New manifest")
    md.add_argument("--format", choices=["text", "md", "json"], default="text")
    md.add_argument("--out", default="", help="Write report to file")
    md.add_argument("--ignore", action="append", default=[], help="Ignore glob(s), can repeat")
    md.add_argument("--only", action="append", default=[], help="Only glob(s), can repeat")
    md.add_argument("--text-diff", action="store_true", help="Append unified text diffs (requires base dirs)")
    md.add_argument("--base-old", default="", help="Base directory for old files")
    md.add_argument("--base-new", default="", help="Base directory for new files")
    md.add_argument("--text-max-bytes", type=int, default=0, help="Max bytes per file in text diff")
    md.add_argument("--diff-context", type=int, default=3, help="Context lines for unified diff")
    md.set_defaults(_fn=cmd_manifest_diff)

    # normalize-lf
    nl = sub.add_parser("normalize-lf", help="Normalize newlines to LF")
    nl.add_argument("--root", required=True, help="Root directory")
    nl.add_argument("--include", action="append", default=[], help="Include glob(s)")
    nl.add_argument("--exclude", action="append", default=[], help="Exclude glob(s)")
    nl.add_argument("--ext", action="append", default=[], help="Extra text extensions")
    nl.add_argument("--encoding", choices=["replace", "strict"], default="replace")
    nl.add_argument("--max-bytes", type=int, default=0, help="Skip files larger than this")
    nl.add_argument("--dry-run", action="store_true")
    nl.set_defaults(_fn=cmd_normalize_lf)

    # sample
    s = sub.add_parser("sample", help="Sample a corpus deterministically")
    s.add_argument("--in", dest="in_dir", required=True, help="Input directory")
    s.add_argument("--out", dest="out_dir", required=True, help="Output directory")
    s.add_argument("--count", type=int, default=0)
    s.add_argument("--bytes", type=int, default=0)
    s.add_argument("--pct", type=float, default=0.0)
    s.add_argument("--seed", default="0")
    s.add_argument("--include", action="append", default=[])
    s.add_argument("--exclude", action="append", default=[])
    s.add_argument("--ext", action="append", default=[])
    s.add_argument("--manifest", default="", help="Write manifest for sampled output")
    s.add_argument("--clean", action="store_true")
    s.add_argument("--dry-run", action="store_true")
    s.set_defaults(_fn=cmd_sample)

    # validate-schema
    vs = sub.add_parser("validate-schema", help="Validate JSON files against a schema")
    vs.add_argument("--schema", required=True)
    vs.add_argument("--in", dest="in_dir", default="")
    vs.add_argument("--files", nargs="*", default=[])
    vs.add_argument("--include", action="append", default=[])
    vs.add_argument("--exclude", action="append", default=[])
    vs.add_argument("--report", default="")
    vs.add_argument("--max-issues", type=int, default=50)
    vs.set_defaults(_fn=cmd_validate_schema)

    return ap


def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = build_parser()
    ns = ap.parse_args(list(argv) if argv is not None else None)

    # propagate global flags if subcommand expects them
    if hasattr(ns, "quiet") and ns.quiet:
        pass

    fn = getattr(ns, "_fn", None)
    if fn is None:
        _eprint("error: missing command handler")
        return 3

    return int(fn(ns))


if __name__ == "__main__":
    raise SystemExit(main())
