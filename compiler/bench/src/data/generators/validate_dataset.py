#!/usr/bin/env python3
"""validate_dataset.py

Validate generated fixture datasets produced by the bench data generators.

Goals
- Provide a deterministic, CI-friendly validator for the generated tree.
- Catch common regressions:
  - missing index.json / README.md
  - malformed JSON
  - missing set artifacts (input.vitte, expected.*.json, meta.json)
  - contract mismatch (name/version)
  - path traversal in reported paths (must stay under generator root)

This validator is intentionally conservative and schema-light:
- It does not attempt to fully validate all sub-structures.
- It validates the invariants required by CI and consumers.

Exit codes
- 0: valid
- 2: usage error
- 3: IO/internal error
- 5: validation failed

Examples
  python3 validate_dataset.py --root ../generated
  python3 validate_dataset.py --root ../generated --manifest ./manifest.json
  python3 validate_dataset.py --root ../generated --only unicode,tokens
  python3 validate_dataset.py --root ../generated --strict
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple


# -----------------------------------------------------------------------------
# Logging
# -----------------------------------------------------------------------------


def _eprint(*a: object) -> None:
    print(*a, file=sys.stderr)


class Log:
    def __init__(self, *, quiet: bool = False, verbose: bool = False):
        self.quiet = quiet
        self.verbose = verbose

    def info(self, *a: object) -> None:
        if not self.quiet:
            print(*a)

    def debug(self, *a: object) -> None:
        if self.verbose and not self.quiet:
            print(*a)

    def error(self, *a: object) -> None:
        _eprint(*a)


# -----------------------------------------------------------------------------
# Manifest model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Contract:
    name: str
    version: int


@dataclass(frozen=True)
class Gen:
    name: str
    out_subdir: str
    script: str
    contract: Optional[Contract]
    optional: bool


def _read_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def load_manifest(path: Path) -> List[Gen]:
    m = _read_json(path)
    if m.get("schema") != "vitte.bench.generators.manifest.v1":
        raise ValueError(f"unsupported manifest schema: {m.get('schema')}")

    gens: List[Gen] = []
    for g in m.get("generators", []):
        supports = g.get("supports", {})
        contract_obj = g.get("contract")
        contract: Optional[Contract] = None
        if isinstance(contract_obj, dict):
            contract = Contract(
                name=str(contract_obj.get("name", "")),
                version=int(contract_obj.get("version", 0)),
            )

        gens.append(
            Gen(
                name=str(g.get("name", "")),
                out_subdir=str(g.get("out_subdir", g.get("name", ""))),
                script=str(g.get("script", "")),
                contract=contract,
                optional=bool(supports.get("optional", False)),
            )
        )

    return gens


# -----------------------------------------------------------------------------
# Validation helpers
# -----------------------------------------------------------------------------


def _is_under(child: Path, parent: Path) -> bool:
    try:
        child.resolve().relative_to(parent.resolve())
        return True
    except Exception:
        return False


def _must_file(p: Path, *, errors: List[str]) -> None:
    if not p.is_file():
        errors.append(f"missing file: {p}")


def _must_dir(p: Path, *, errors: List[str]) -> None:
    if not p.is_dir():
        errors.append(f"missing dir: {p}")


def _load_json(p: Path, *, errors: List[str]) -> Optional[Dict[str, Any]]:
    if not p.is_file():
        errors.append(f"missing file: {p}")
        return None
    try:
        return _read_json(p)
    except Exception as e:
        errors.append(f"invalid json: {p}: {e}")
        return None


def _as_int(x: Any, default: int = 0) -> int:
    try:
        return int(x)
    except Exception:
        return default


def _as_str(x: Any, default: str = "") -> str:
    try:
        return str(x)
    except Exception:
        return default


def _valid_case_dir_name(name: str) -> bool:
    # Expect: <hex16>_<anything>
    if len(name) < 18:
        return False
    if name[16] != "_":
        return False
    return bool(re.fullmatch(r"[0-9a-f]{16}_[^/\\]+", name))


# -----------------------------------------------------------------------------
# Core validation
# -----------------------------------------------------------------------------


def validate_generator_root(
    *,
    gen: Gen,
    gen_root: Path,
    strict: bool,
    log: Log,
    errors: List[str],
) -> None:
    log.debug(f"[validate] {gen.name} -> {gen_root}")

    _must_dir(gen_root, errors=errors)

    idx_path = gen_root / "index.json"
    readme_path = gen_root / "README.md"

    _must_file(idx_path, errors=errors)
    _must_file(readme_path, errors=errors)

    idx = _load_json(idx_path, errors=errors)
    if idx is None:
        return

    # Validate contract (if declared)
    if gen.contract is not None:
        c = idx.get("contract")
        if not isinstance(c, dict):
            errors.append(f"{gen.name}: index.json missing 'contract' object")
        else:
            cn = _as_str(c.get("name"))
            cv = _as_int(c.get("version"))
            if cn != gen.contract.name or cv != gen.contract.version:
                errors.append(
                    f"{gen.name}: contract mismatch: index has {cn}@{cv}, manifest wants {gen.contract.name}@{gen.contract.version}"
                )

    # Validate sets (if present)
    sets = idx.get("sets")
    if sets is None:
        # Some generators may not have sets.
        return

    if not isinstance(sets, list):
        errors.append(f"{gen.name}: index.json 'sets' must be a list")
        return

    for i, s in enumerate(sets):
        if not isinstance(s, dict):
            errors.append(f"{gen.name}: index.json sets[{i}] must be an object")
            continue

        rel_path = _as_str(s.get("path"))
        if rel_path == "":
            errors.append(f"{gen.name}: sets[{i}] missing 'path'")
            continue

        # Normalize path separators
        rel_path_norm = rel_path.replace("\\", "/")
        if rel_path_norm.startswith("/") or ".." in rel_path_norm.split("/"):
            errors.append(f"{gen.name}: sets[{i}] invalid path (traversal): {rel_path}")
            continue

        set_dir = gen_root / rel_path_norm
        if not _is_under(set_dir, gen_root):
            errors.append(f"{gen.name}: sets[{i}] path escapes generator root: {rel_path}")
            continue

        # Expect directory and standard artifacts
        _must_dir(set_dir, errors=errors)
        _must_file(set_dir / "input.vitte", errors=errors)
        _must_file(set_dir / "meta.json", errors=errors)

        # expected.*.json can vary by generator; accept a few well-known names.
        expected_candidates = [
            set_dir / "expected.tokens.json",
            set_dir / "expected.trivia.json",
            set_dir / "expected.unicode.json",
            set_dir / "expected.sema.json",
        ]
        if not any(p.is_file() for p in expected_candidates):
            errors.append(f"{gen.name}: sets[{i}] missing expected.*.json in {set_dir}")

        # Validate meta.json minimally
        meta = _load_json(set_dir / "meta.json", errors=errors)
        if meta is not None:
            mid = _as_str(meta.get("id"))
            mname = _as_str(meta.get("name"))
            if strict:
                # Strict mode expects these fields.
                if not mid or not mname:
                    errors.append(f"{gen.name}: meta.json missing required fields (id/name) in {set_dir}")

        # Optional: validate directory naming convention if set_dir is under sets/
        parts = Path(rel_path_norm).parts
        if len(parts) >= 2 and parts[0] in ("sets", "reports"):
            leaf = parts[-1]
            if not _valid_case_dir_name(leaf):
                # reports may include different naming; only enforce for sets.
                if parts[0] == "sets":
                    errors.append(f"{gen.name}: unexpected set directory name: {leaf}")


def validate_root(
    *,
    root: Path,
    manifest: Path,
    only: Sequence[str],
    skip: Sequence[str],
    strict: bool,
    log: Log,
) -> Tuple[bool, List[str]]:
    errors: List[str] = []

    gens = load_manifest(manifest)

    only_set = set([x for x in only if x])
    skip_set = set([x for x in skip if x])

    selected: List[Gen] = []
    for g in gens:
        if only_set and g.name not in only_set:
            continue
        if g.name in skip_set:
            continue
        selected.append(g)

    if not selected:
        errors.append("no generators selected")
        return False, errors

    if not root.exists():
        errors.append(f"root does not exist: {root}")
        return False, errors

    if not root.is_dir():
        errors.append(f"root is not a directory: {root}")
        return False, errors

    for g in selected:
        gen_root = root / g.out_subdir
        if not gen_root.exists():
            if g.optional:
                log.debug(f"[skip] optional generator output missing: {g.name}")
                continue
            errors.append(f"missing generator output directory: {gen_root}")
            continue

        validate_generator_root(gen=g, gen_root=gen_root, strict=strict, log=log, errors=errors)

    return len(errors) == 0, errors


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def _csv_list(s: str) -> List[str]:
    if not s:
        return []
    return [x.strip() for x in s.split(",") if x.strip()]


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="validate_dataset.py")
    ap.add_argument("--root", required=True, help="Generated dataset root (e.g. ../generated)")
    ap.add_argument("--manifest", default="manifest.json", help="Manifest JSON path")
    ap.add_argument("--only", default="", help="Comma-separated generator names to validate")
    ap.add_argument("--skip", default="", help="Comma-separated generator names to skip")
    ap.add_argument("--strict", action="store_true", help="Enable stricter invariants")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    return ap.parse_args(list(argv))


def main(argv: Optional[List[str]] = None) -> int:
    ns = parse_args(argv if argv is not None else sys.argv[1:])

    here = Path(__file__).resolve().parent
    root = Path(ns.root).expanduser().resolve()
    manifest = (here / ns.manifest).expanduser().resolve()

    if not manifest.is_file():
        _eprint(f"error: manifest not found: {manifest}")
        return 3

    only = _csv_list(str(ns.only))
    skip = _csv_list(str(ns.skip))

    log = Log(quiet=bool(ns.quiet), verbose=bool(ns.verbose))

    try:
        ok, errs = validate_root(
            root=root,
            manifest=manifest,
            only=only,
            skip=skip,
            strict=bool(ns.strict),
            log=log,
        )
    except Exception as e:
        _eprint(f"error: validate failed: {e}")
        return 3

    if ok:
        if not ns.quiet:
            print("OK")
        return 0

    # Print errors
    for e in errs:
        log.error(f"FAIL: {e}")

    return 5


if __name__ == "__main__":
    raise SystemExit(main())
