#!/usr/bin/env python3
"""gen_index.py

Index generator for `compiler/bench/src/data/generators`.

This tool builds a deterministic index describing the generator workspace:
- python entrypoints (top-level scripts)
- tools scripts (tools/*.py)
- vitte_benchgen package metadata (version if available)

The index is useful for:
- CI artifact inventories
- repo auditing (what runs where)
- discoverability and documentation

Outputs:
- JSON (default)
- Markdown report (optional)

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO error

Examples:
  python3 gen_index.py --root . --out index.json
  python3 gen_index.py --root . --format md --out INDEX.md
  python3 gen_index.py --root . --hash --out index.json
  python3 gen_index.py --root . --include "**/*.py" --exclude "**/__pycache__/**"
"""

from __future__ import annotations

import argparse
import fnmatch
import hashlib
import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple


# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------


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


def match_any(path_posix: str, globs: List[str]) -> bool:
    return any(fnmatch.fnmatch(path_posix, g) for g in globs)


def stable_walk(root: Path) -> Iterable[Path]:
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        filenames.sort()
        for fn in filenames:
            yield Path(dirpath) / fn


def rel_posix(path: Path, root: Path) -> str:
    rp = path.resolve().as_posix()
    rr = root.resolve().as_posix().rstrip("/") + "/"
    if rp.startswith(rr):
        return rp[len(rr) :]
    return path.as_posix().replace("\\", "/")


def sha256_file(path: Path, chunk: int = 1024 * 1024) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


# -----------------------------------------------------------------------------
# Index building
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class FileRow:
    path: str
    kind: str
    bytes: int
    sha256: str


def _kind_for(rel: str) -> str:
    # Very small, stable taxonomy.
    if rel.endswith(".py"):
        if rel.startswith("tools/"):
            return "tool"
        if rel.startswith("vitte_benchgen/"):
            return "lib"
        return "entry"
    if rel.endswith(".md"):
        return "doc"
    if rel.endswith(".json"):
        return "data"
    return "file"


def build_index(
    root: Path,
    *,
    include: List[str],
    exclude: List[str],
    hash_files: bool,
    max_bytes: int,
) -> Dict[str, Any]:
    files: List[FileRow] = []

    for p in stable_walk(root):
        if not p.is_file():
            continue
        # Ignore symlinks by default
        try:
            if p.is_symlink():
                continue
        except OSError:
            continue

        rel = rel_posix(p, root)

        if exclude and match_any(rel, exclude):
            continue
        if include:
            if not match_any(rel, include):
                continue

        try:
            st = p.stat()
            sz = int(st.st_size)
        except OSError:
            continue

        digest = ""
        if hash_files and sz <= max_bytes:
            try:
                digest = sha256_file(p)
            except OSError:
                digest = ""

        files.append(FileRow(path=rel, kind=_kind_for(rel), bytes=sz, sha256=digest))

    # Deterministic sort.
    files.sort(key=lambda r: (r.kind, r.path))

    # Attempt to read package metadata.
    pkg: Dict[str, Any] = {"name": "vitte_benchgen", "version": ""}
    try:
        # Ensure local import works when executed in-repo.
        if str(root) not in sys.path:
            sys.path.insert(0, str(root))
        import vitte_benchgen  # type: ignore

        pkg["version"] = getattr(vitte_benchgen, "__version__", "")
    except Exception:
        pkg["version"] = ""

    payload: Dict[str, Any] = {
        "root": root.as_posix(),
        "package": pkg,
        "counts": {
            "total": len(files),
            "entry": sum(1 for f in files if f.kind == "entry"),
            "tool": sum(1 for f in files if f.kind == "tool"),
            "lib": sum(1 for f in files if f.kind == "lib"),
        },
        "files": [
            {
                "path": f.path,
                "kind": f.kind,
                "bytes": f.bytes,
                **({"sha256": f.sha256} if hash_files else {}),
            }
            for f in files
        ],
    }

    return payload


# -----------------------------------------------------------------------------
# Output
# -----------------------------------------------------------------------------


def dumps_json_stable(obj: Any, indent: int = 2) -> str:
    import json

    return json.dumps(
        obj,
        ensure_ascii=False,
        sort_keys=True,
        indent=indent,
        separators=(",", ": ") if indent else (",", ":"),
    )


def emit_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []

    pkg = index.get("package", {})
    counts = index.get("counts", {})

    lines.append("# Generators index")
    lines.append("")
    lines.append(f"- root: `{index.get('root', '')}`")
    lines.append(f"- package: `{pkg.get('name', '')}` version `{pkg.get('version', '')}`")
    lines.append("")
    lines.append("## Counts")
    lines.append("")
    lines.append(f"- total: **{counts.get('total', 0)}**")
    lines.append(f"- entry: **{counts.get('entry', 0)}**")
    lines.append(f"- tool: **{counts.get('tool', 0)}**")
    lines.append(f"- lib: **{counts.get('lib', 0)}**")

    def table(kind: str) -> None:
        rows = [f for f in index.get("files", []) if f.get("kind") == kind]
        lines.append("")
        lines.append(f"## {kind}")
        lines.append("")
        if not rows:
            lines.append("_none_")
            return
        cols = ["path", "bytes"]
        has_hash = any("sha256" in r for r in rows)
        if has_hash:
            cols.append("sha256")
        lines.append("| " + " | ".join(cols) + " |")
        lines.append("|" + "|".join(["---"] * len(cols)) + "|")
        for r in rows:
            path = r.get("path", "")
            b = str(r.get("bytes", 0))
            if has_hash:
                sha = str(r.get("sha256", ""))
                sha = sha[:12] if sha else ""
                lines.append(f"| `{path}` | {b} | `{sha}` |")
            else:
                lines.append(f"| `{path}` | {b} |")

    table("entry")
    table("tool")
    table("lib")

    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_index.py")
    ap.add_argument("--root", default=".", help="Root directory (default: .)")
    ap.add_argument("--out", default="index.json", help="Output path")
    ap.add_argument("--format", choices=["json", "md"], default="json")

    ap.add_argument(
        "--include",
        action="append",
        default=[],
        help="Include glob(s) relative to --root (comma-separated allowed)",
    )
    ap.add_argument(
        "--exclude",
        action="append",
        default=[],
        help="Exclude glob(s) relative to --root (comma-separated allowed)",
    )

    ap.add_argument("--hash", action="store_true", help="Include sha256 per file")
    ap.add_argument(
        "--max-bytes",
        type=int,
        default=8 * 1024 * 1024,
        help="Max bytes hashed per file (default: 8 MiB)",
    )

    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--verbose", action="store_true")

    return ap.parse_args(list(argv))


def main(argv: Optional[List[str]] = None) -> int:
    ns = parse_args(sys.argv[1:] if argv is None else argv)

    root = Path(ns.root).expanduser().resolve()
    out = Path(ns.out).expanduser().resolve()

    if not root.exists() or not root.is_dir():
        eprint(f"error: --root not a directory: {root}")
        return 2

    include = split_globs(ns.include)
    exclude = split_globs(ns.exclude)

    # Safe defaults
    if not include:
        # index python sources + key docs by default
        include = ["*.py", "tools/*.py", "vitte_benchgen/**/*.py", "README.md", "LICENSE", "*.md"]

    try:
        index = build_index(
            root,
            include=include,
            exclude=exclude,
            hash_files=bool(ns.hash),
            max_bytes=int(ns.max_bytes),
        )

        out.parent.mkdir(parents=True, exist_ok=True)

        if ns.format == "json":
            out.write_text(dumps_json_stable(index, indent=2) + "\n", encoding="utf-8")
        else:
            out.write_text(emit_md(index), encoding="utf-8")

    except OSError as e:
        eprint(f"io error: {e}")
        return 3

    if not ns.quiet:
        counts = index.get("counts", {})
        print(
            f"gen_index: root={root} out={out} total={counts.get('total', 0)} entry={counts.get('entry', 0)} tool={counts.get('tool', 0)} lib={counts.get('lib', 0)}"
        )

    if ns.verbose and not ns.quiet:
        for f in index.get("files", []):
            print(f"  [{f.get('kind')}] {f.get('path')} ({f.get('bytes')})")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
