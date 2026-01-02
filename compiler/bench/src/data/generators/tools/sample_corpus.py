#!/usr/bin/env python3
"""sample_corpus.py

Corpus sampler for Vitte bench/data.

Problem:
- Generated corpora can be huge; CI or local debugging often needs a deterministic sample.

This tool:
- Selects a deterministic subset of files from an input directory.
- Writes them into an output directory, preserving relative paths.
- Optionally generates a manifest JSON with sha256/size.
- Can sample by count, total bytes, or percentage.
- Supports include/exclude globs, extension filters, and seed.

Exit codes:
- 0 success
- 2 usage/input error
- 3 IO error

Examples:
  python3 sample_corpus.py --in ../generated --out ../sampled --count 200
  python3 sample_corpus.py --in ../generated --out ../sampled --pct 0.05 --seed 123
  python3 sample_corpus.py --in ../generated --out ../sampled --bytes 2000000 --include "**/*.vitte"
  python3 sample_corpus.py --in ../generated --out ../sampled --count 100 --manifest sampled_manifest.json

Determinism:
- Files are enumerated in a stable order.
- Selection uses a stable hash (sha256) of (seed + relative path) then sorts by that.
- That makes the sample stable across platforms and runs.
"""

from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import os
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


DEFAULT_TEXT_EXT = {
    ".vitte",
    ".vit",
    ".v",
    ".c",
    ".h",
    ".md",
    ".txt",
    ".json",
    ".yml",
    ".yaml",
    ".toml",
    ".py",
    ".lua",
    ".sh",
    ".ps1",
}


@dataclass(frozen=True)
class FileInfo:
    rel: str
    abs: Path
    size: int


def eprint(*a: object) -> None:
    print(*a, file=sys.stderr)


def split_globs(items: List[str]) -> List[str]:
    out: List[str] = []
    for it in items:
        if not it:
            continue
        for p in it.split(","):
            p = p.strip()
            if p:
                out.append(p)
    return out


def match_any(rel_posix: str, globs: List[str]) -> bool:
    return any(fnmatch.fnmatch(rel_posix, g) for g in globs)


def iter_files(root: Path) -> Iterable[Path]:
    # Deterministic traversal.
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        filenames.sort()
        for fn in filenames:
            yield Path(dirpath) / fn


def is_eligible(p: Path, rel_posix: str, include: List[str], exclude: List[str], exts: set[str]) -> bool:
    if p.is_symlink():
        return False
    if exclude and match_any(rel_posix, exclude):
        return False
    if include:
        return match_any(rel_posix, include)
    return p.suffix.lower() in exts


def stable_rank(seed: str, rel_posix: str) -> bytes:
    h = hashlib.sha256()
    h.update(seed.encode("utf-8"))
    h.update(b"\n")
    h.update(rel_posix.encode("utf-8"))
    return h.digest()


def sha256_file(path: Path, chunk: int = 1024 * 1024) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    # copy2 preserves timestamps where possible
    shutil.copy2(src, dst)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="sample_corpus.py")
    ap.add_argument("--in", dest="in_dir", required=True, help="Input corpus directory")
    ap.add_argument("--out", dest="out_dir", required=True, help="Output directory")

    sel = ap.add_argument_group("selection")
    sel.add_argument("--count", type=int, default=0, help="Max number of files")
    sel.add_argument("--bytes", type=int, default=0, help="Max total bytes")
    sel.add_argument("--pct", type=float, default=0.0, help="Fraction of eligible files (0..1)")

    ap.add_argument("--seed", type=str, default="0", help="Seed for deterministic sampling")

    ap.add_argument(
        "--include",
        action="append",
        default=[],
        help="Glob(s) to include (can repeat; comma-separated allowed). If set, overrides extension filter.",
    )
    ap.add_argument(
        "--exclude",
        action="append",
        default=[],
        help="Glob(s) to exclude (can repeat; comma-separated allowed).",
    )

    ap.add_argument(
        "--ext",
        action="append",
        default=[],
        help="Additional file extensions to treat as eligible when --include is not used.",
    )

    ap.add_argument(
        "--manifest",
        type=str,
        default="",
        help="Write manifest JSON for sampled files (path/bytes/sha256)",
    )

    ap.add_argument("--clean", action="store_true", help="Delete output directory before writing")
    ap.add_argument("--dry-run", action="store_true", help="Do not copy, only report")
    ap.add_argument("--verbose", action="store_true", help="Print selected files")
    ap.add_argument("--quiet", action="store_true", help="Only print summary")

    return ap.parse_args(list(argv))


def main(argv: Optional[List[str]] = None) -> int:
    ns = parse_args(sys.argv[1:] if argv is None else argv)

    in_root = Path(ns.in_dir).expanduser().resolve()
    out_root = Path(ns.out_dir).expanduser().resolve()

    if not in_root.exists() or not in_root.is_dir():
        eprint(f"error: input directory does not exist: {in_root}")
        return 2

    include = split_globs(ns.include)
    exclude = split_globs(ns.exclude)

    exts = set(DEFAULT_TEXT_EXT)
    for e in ns.ext:
        if not e:
            continue
        x = e.strip()
        if not x:
            continue
        if not x.startswith("."):
            x = "." + x
        exts.add(x.lower())

    # selection constraints
    if ns.count < 0 or ns.bytes < 0 or ns.pct < 0.0 or ns.pct > 1.0:
        eprint("error: invalid selection constraints")
        return 2

    if ns.count == 0 and ns.bytes == 0 and ns.pct == 0.0:
        # Default: small but useful sample.
        ns.count = 200

    if ns.clean and out_root.exists() and out_root.is_dir() and not ns.dry_run:
        shutil.rmtree(out_root)

    # Enumerate eligible files
    eligible: List[FileInfo] = []

    in_posix = in_root.as_posix().rstrip("/") + "/"

    def rel_posix(p: Path) -> str:
        ap = p.resolve().as_posix()
        if ap.startswith(in_posix):
            return ap[len(in_posix) :]
        return p.as_posix()

    try:
        for p in iter_files(in_root):
            if not p.is_file():
                continue
            rel = rel_posix(p)
            if is_eligible(p, rel, include=include, exclude=exclude, exts=exts):
                try:
                    sz = p.stat().st_size
                except OSError:
                    continue
                eligible.append(FileInfo(rel=rel, abs=p, size=int(sz)))
    except OSError as e:
        eprint(f"io error during traversal: {e}")
        return 3

    if not eligible:
        eprint("warning: no eligible files")
        # still create empty output dir if not dry-run
        if not ns.dry_run:
            out_root.mkdir(parents=True, exist_ok=True)
        return 0

    # Determine count from pct
    target_count = ns.count
    if ns.pct > 0.0:
        pct_count = int(round(len(eligible) * ns.pct))
        if pct_count < 1 and ns.pct > 0.0:
            pct_count = 1
        if target_count == 0:
            target_count = pct_count
        else:
            target_count = min(target_count, pct_count)

    # Rank deterministically and sort
    seed = str(ns.seed)
    ranked = sorted(
        eligible,
        key=lambda fi: stable_rank(seed, fi.rel),
    )

    selected: List[FileInfo] = []
    total_bytes = 0

    for fi in ranked:
        if target_count and len(selected) >= target_count:
            break
        if ns.bytes and (total_bytes + fi.size) > ns.bytes:
            # cannot add without exceeding budget; continue scanning
            continue
        selected.append(fi)
        total_bytes += fi.size

    # If bytes-only selection resulted in empty due to a single huge file,
    # allow selecting the smallest file.
    if not selected and ranked:
        smallest = min(ranked, key=lambda x: x.size)
        selected = [smallest]
        total_bytes = smallest.size

    if not ns.quiet:
        print(
            f"sample_corpus: in={in_root} out={out_root} eligible={len(eligible)} selected={len(selected)} bytes={total_bytes} seed={seed}"
        )

    if ns.verbose and not ns.quiet:
        for fi in selected:
            print(f"  {fi.rel} ({fi.size})")

    if ns.dry_run:
        return 0

    # Copy
    out_root.mkdir(parents=True, exist_ok=True)

    manifest_entries: List[Dict[str, object]] = []

    for fi in selected:
        dst = out_root / fi.rel
        try:
            copy_file(fi.abs, dst)
        except OSError as e:
            eprint(f"io error copying {fi.rel}: {e}")
            return 3

        if ns.manifest:
            try:
                digest = sha256_file(dst)
            except OSError as e:
                eprint(f"io error hashing {fi.rel}: {e}")
                return 3

            manifest_entries.append(
                {
                    "path": fi.rel,
                    "bytes": fi.size,
                    "sha256": digest,
                }
            )

    if ns.manifest:
        try:
            mp = Path(ns.manifest)
            mp.parent.mkdir(parents=True, exist_ok=True)
            payload = {
                "tool": "sample_corpus.py",
                "seed": seed,
                "input": str(in_root),
                "output": str(out_root),
                "eligible": len(eligible),
                "selected": len(selected),
                "bytes": total_bytes,
                "files": manifest_entries,
            }
            mp.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        except OSError as e:
            eprint(f"io error writing manifest: {e}")
            return 3

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
