

#!/usr/bin/env python3
"""manifest_diff.py

Enterprise-grade manifest differ for Vitte bench/data generators.

Typical use-cases:
- Compare two generated corpus/fixture manifests (e.g. `manifest.json`) across commits.
- Produce human-readable or machine-readable reports for CI.

Expected manifest shape (flexible):
- { "files": [ {"path": "...", "bytes": 123, "sha256": "...", ...}, ... ], ... }
- or simply [ {"path": "...", ...}, ... ]

Exit codes:
- 0: no differences
- 1: differences found
- 2: usage / input error

Examples:
  python3 manifest_diff.py old/manifest.json new/manifest.json
  python3 manifest_diff.py old.json new.json --format md --out report.md
  python3 manifest_diff.py old.json new.json --format json --fail-on-diff
  python3 manifest_diff.py old.json new.json --base-old old_tree --base-new new_tree --text-diff

Notes:
- By default, compares entries by `path`, and uses `sha256` when available.
- Falls back to `bytes`/`size` when sha256 is missing.
"""

from __future__ import annotations

import argparse
import difflib
import fnmatch
import json
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple


@dataclass(frozen=True)
class Entry:
    path: str
    sha256: str
    size: int
    raw: Dict[str, Any]


@dataclass
class DiffResult:
    added: List[Entry]
    removed: List[Entry]
    modified: List[Tuple[Entry, Entry]]
    unchanged: int


def _eprint(*a: Any) -> None:
    print(*a, file=sys.stderr)


def _read_json(path: Path) -> Any:
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        raise SystemExit(f"input not found: {path}")
    except json.JSONDecodeError as e:
        raise SystemExit(f"invalid JSON: {path}: {e}")


def _normalize_manifest(obj: Any) -> List[Dict[str, Any]]:
    """Return list of dict entries."""
    if isinstance(obj, list):
        if all(isinstance(x, dict) for x in obj):
            return obj
        raise SystemExit("manifest JSON list must contain objects")

    if isinstance(obj, dict):
        files = obj.get("files")
        if files is None:
            # allow a mapping path -> entry
            # e.g. {"a.txt": {"sha256":..., "bytes":...}, ...}
            if all(isinstance(k, str) and isinstance(v, dict) for k, v in obj.items()):
                out: List[Dict[str, Any]] = []
                for k, v in obj.items():
                    vv = dict(v)
                    vv.setdefault("path", k)
                    out.append(vv)
                return out
            raise SystemExit("manifest object must contain 'files' or be a mapping of path->entry")

        if not isinstance(files, list) or not all(isinstance(x, dict) for x in files):
            raise SystemExit("manifest['files'] must be a list of objects")
        return files

    raise SystemExit("manifest JSON must be an object or an array")


def _get_int(d: Dict[str, Any], *keys: str, default: int = 0) -> int:
    for k in keys:
        v = d.get(k)
        if isinstance(v, int):
            return v
        if isinstance(v, str) and v.isdigit():
            return int(v)
    return default


def _get_str(d: Dict[str, Any], *keys: str, default: str = "") -> str:
    for k in keys:
        v = d.get(k)
        if isinstance(v, str):
            return v
    return default


def _to_entry(d: Dict[str, Any]) -> Entry:
    p = _get_str(d, "path")
    if not p:
        raise SystemExit("manifest entry missing 'path'")

    # normalize separators
    p = p.replace("\\", "/")

    sha = _get_str(d, "sha256", "hash", "digest", default="")
    size = _get_int(d, "bytes", "size", "len", default=0)

    # if sha is absent, allow size-only but keep stable representation
    return Entry(path=p, sha256=sha, size=size, raw=dict(d))


def _apply_filters(entries: Iterable[Entry], ignore_globs: List[str], only_globs: List[str]) -> List[Entry]:
    out: List[Entry] = []

    def ignored(p: str) -> bool:
        return any(fnmatch.fnmatch(p, g) for g in ignore_globs)

    def allowed(p: str) -> bool:
        if not only_globs:
            return True
        return any(fnmatch.fnmatch(p, g) for g in only_globs)

    for e in entries:
        if ignored(e.path):
            continue
        if not allowed(e.path):
            continue
        out.append(e)

    return out


def _index(entries: Iterable[Entry]) -> Dict[str, Entry]:
    m: Dict[str, Entry] = {}
    for e in entries:
        # if duplicates, last wins but warn
        if e.path in m:
            _eprint(f"warning: duplicate path in manifest: {e.path}")
        m[e.path] = e
    return m


def _same(a: Entry, b: Entry) -> bool:
    # Prefer sha256 when present in both.
    if a.sha256 and b.sha256:
        return a.sha256 == b.sha256
    # Otherwise compare sizes.
    return a.size == b.size


_TEXT_EXT = {
    ".c",
    ".h",
    ".md",
    ".txt",
    ".json",
    ".yml",
    ".yaml",
    ".toml",
    ".ini",
    ".cfg",
    ".sh",
    ".ps1",
    ".vitte",
    ".vit",
    ".v",
    ".py",
    ".lua",
}


def _looks_text(path: Path) -> bool:
    if path.suffix.lower() in _TEXT_EXT:
        return True
    # heuristic: no NUL bytes in first chunk
    try:
        with path.open("rb") as f:
            chunk = f.read(4096)
        return b"\x00" not in chunk
    except OSError:
        return False


def _read_text(path: Path, limit_bytes: int) -> Optional[List[str]]:
    try:
        bs = path.read_bytes()
    except OSError:
        return None
    if len(bs) > limit_bytes:
        return None
    try:
        txt = bs.decode("utf-8")
    except UnicodeDecodeError:
        return None
    # normalize newlines
    txt = txt.replace("\r\n", "\n").replace("\r", "\n")
    return txt.splitlines(keepends=True)


def diff_manifests(old: List[Entry], new: List[Entry]) -> DiffResult:
    mo = _index(old)
    mn = _index(new)

    old_paths = set(mo.keys())
    new_paths = set(mn.keys())

    added = [mn[p] for p in sorted(new_paths - old_paths)]
    removed = [mo[p] for p in sorted(old_paths - new_paths)]

    modified: List[Tuple[Entry, Entry]] = []
    unchanged = 0

    for p in sorted(old_paths & new_paths):
        a = mo[p]
        b = mn[p]
        if _same(a, b):
            unchanged += 1
        else:
            modified.append((a, b))

    return DiffResult(added=added, removed=removed, modified=modified, unchanged=unchanged)


def _fmt_entry(e: Entry) -> str:
    sha = e.sha256[:12] if e.sha256 else "(no-sha)"
    return f"{e.path}  size={e.size}  sha={sha}"


def _emit_text(res: DiffResult) -> str:
    lines: List[str] = []
    total_old = res.removed.__len__() + res.modified.__len__() + res.unchanged
    total_new = res.added.__len__() + res.modified.__len__() + res.unchanged

    lines.append(f"summary: old={total_old} new={total_new} unchanged={res.unchanged} modified={len(res.modified)} added={len(res.added)} removed={len(res.removed)}")

    if res.added:
        lines.append("")
        lines.append("added:")
        for e in res.added:
            lines.append("  + " + _fmt_entry(e))

    if res.removed:
        lines.append("")
        lines.append("removed:")
        for e in res.removed:
            lines.append("  - " + _fmt_entry(e))

    if res.modified:
        lines.append("")
        lines.append("modified:")
        for a, b in res.modified:
            a_sha = a.sha256[:12] if a.sha256 else "(no-sha)"
            b_sha = b.sha256[:12] if b.sha256 else "(no-sha)"
            lines.append(f"  * {a.path}")
            lines.append(f"      old: size={a.size} sha={a_sha}")
            lines.append(f"      new: size={b.size} sha={b_sha}")

    return "\n".join(lines) + "\n"


def _emit_md(res: DiffResult) -> str:
    lines: List[str] = []
    total_old = res.removed.__len__() + res.modified.__len__() + res.unchanged
    total_new = res.added.__len__() + res.modified.__len__() + res.unchanged

    lines.append("# Manifest diff")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- old: **{total_old}**")
    lines.append(f"- new: **{total_new}**")
    lines.append(f"- unchanged: **{res.unchanged}**")
    lines.append(f"- modified: **{len(res.modified)}**")
    lines.append(f"- added: **{len(res.added)}**")
    lines.append(f"- removed: **{len(res.removed)}**")

    def table(title: str, rows: List[List[str]]) -> None:
        lines.append("")
        lines.append(f"## {title}")
        lines.append("")
        if not rows:
            lines.append("_none_")
            return
        lines.append("| path | size | sha |")
        lines.append("|---|---:|---|")
        for r in rows:
            lines.append(f"| `{r[0]}` | {r[1]} | `{r[2]}` |")

    table(
        "Added",
        [[e.path, str(e.size), (e.sha256[:12] if e.sha256 else "(no-sha)")] for e in res.added],
    )
    table(
        "Removed",
        [[e.path, str(e.size), (e.sha256[:12] if e.sha256 else "(no-sha)")] for e in res.removed],
    )

    lines.append("")
    lines.append("## Modified")
    lines.append("")
    if not res.modified:
        lines.append("_none_")
    else:
        lines.append("| path | old size | new size | old sha | new sha |")
        lines.append("|---|---:|---:|---|---|")
        for a, b in res.modified:
            lines.append(
                f"| `{a.path}` | {a.size} | {b.size} | `{(a.sha256[:12] if a.sha256 else '(no-sha)')}` | `{(b.sha256[:12] if b.sha256 else '(no-sha)')}` |"
            )

    return "\n".join(lines) + "\n"


def _emit_json(res: DiffResult) -> str:
    def enc_entry(e: Entry) -> Dict[str, Any]:
        out = {
            "path": e.path,
            "bytes": e.size,
            "sha256": e.sha256,
        }
        # Keep extra keys if present, but do not overwrite.
        for k, v in e.raw.items():
            out.setdefault(k, v)
        return out

    payload: Dict[str, Any] = {
        "summary": {
            "unchanged": res.unchanged,
            "modified": len(res.modified),
            "added": len(res.added),
            "removed": len(res.removed),
        },
        "added": [enc_entry(e) for e in res.added],
        "removed": [enc_entry(e) for e in res.removed],
        "modified": [
            {
                "path": a.path,
                "old": enc_entry(a),
                "new": enc_entry(b),
            }
            for (a, b) in res.modified
        ],
    }
    return json.dumps(payload, ensure_ascii=False, indent=2) + "\n"


def _emit_text_diffs(
    modified: List[Tuple[Entry, Entry]],
    base_old: Optional[Path],
    base_new: Optional[Path],
    max_bytes: int,
    context: int,
) -> str:
    if not modified:
        return ""
    if base_old is None or base_new is None:
        return ""

    blocks: List[str] = []

    for a, b in modified:
        po = base_old / a.path
        pn = base_new / b.path
        if not po.is_file() or not pn.is_file():
            continue
        if not (_looks_text(po) and _looks_text(pn)):
            continue

        ao = _read_text(po, limit_bytes=max_bytes)
        an = _read_text(pn, limit_bytes=max_bytes)
        if ao is None or an is None:
            continue

        diff = difflib.unified_diff(
            ao,
            an,
            fromfile=str(po),
            tofile=str(pn),
            n=context,
        )
        d = "".join(diff)
        if not d:
            continue

        blocks.append("\n".join(["", "---", f"diff: {a.path}", "```diff", d.rstrip("\n"), "```", ""]))

    return "\n".join(blocks)


def _parse_globs(items: List[str]) -> List[str]:
    out: List[str] = []
    for it in items:
        if not it:
            continue
        # allow comma-separated
        parts = [p.strip() for p in it.split(",") if p.strip()]
        out.extend(parts)
    return out


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser(prog="manifest_diff.py", add_help=True)
    ap.add_argument("old", type=str, help="Old manifest JSON")
    ap.add_argument("new", type=str, help="New manifest JSON")
    ap.add_argument("--format", choices=["text", "md", "json"], default="text", help="Output format")
    ap.add_argument("--out", type=str, default="", help="Write report to file (default: stdout)")

    ap.add_argument(
        "--ignore",
        action="append",
        default=[],
        help="Glob(s) to ignore (can repeat; comma-separated allowed), e.g. '*.tmp, out/*'",
    )
    ap.add_argument(
        "--only",
        action="append",
        default=[],
        help="Only include paths matching glob(s) (can repeat; comma-separated allowed)",
    )

    ap.add_argument(
        "--base-old",
        type=str,
        default="",
        help="If provided, enables optional text diffs for modified entries using files under this directory",
    )
    ap.add_argument(
        "--base-new",
        type=str,
        default="",
        help="If provided, enables optional text diffs for modified entries using files under this directory",
    )
    ap.add_argument(
        "--text-diff",
        action="store_true",
        help="Append unified diffs for modified text files (requires --base-old/--base-new)",
    )
    ap.add_argument(
        "--text-max-bytes",
        type=int,
        default=256 * 1024,
        help="Max bytes per file to include in text diff (default: 256 KiB)",
    )
    ap.add_argument(
        "--diff-context",
        type=int,
        default=3,
        help="Unified diff context lines (default: 3)",
    )

    ap.add_argument(
        "--fail-on-diff",
        action="store_true",
        help="Return exit code 1 if differences are found (default: still returns 1 on diff)",
    )
    ap.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress stderr warnings (duplicate paths, etc.)",
    )

    ns = ap.parse_args(argv)

    if ns.quiet:
        global _eprint

        def _eprint(*a: Any) -> None:
            return

    old_path = Path(ns.old)
    new_path = Path(ns.new)

    ignore_globs = _parse_globs(ns.ignore)
    only_globs = _parse_globs(ns.only)

    old_obj = _read_json(old_path)
    new_obj = _read_json(new_path)

    old_entries = [_to_entry(d) for d in _normalize_manifest(old_obj)]
    new_entries = [_to_entry(d) for d in _normalize_manifest(new_obj)]

    old_entries = _apply_filters(old_entries, ignore_globs=ignore_globs, only_globs=only_globs)
    new_entries = _apply_filters(new_entries, ignore_globs=ignore_globs, only_globs=only_globs)

    res = diff_manifests(old_entries, new_entries)

    if ns.format == "text":
        out = _emit_text(res)
    elif ns.format == "md":
        out = _emit_md(res)
    else:
        out = _emit_json(res)

    if ns.text_diff:
        bo = Path(ns.base_old) if ns.base_old else None
        bn = Path(ns.base_new) if ns.base_new else None
        td = _emit_text_diffs(
            res.modified,
            base_old=bo,
            base_new=bn,
            max_bytes=int(ns.text_max_bytes),
            context=int(ns.diff_context),
        )
        if td:
            out = out.rstrip("\n") + "\n" + td

    if ns.out:
        Path(ns.out).parent.mkdir(parents=True, exist_ok=True)
        Path(ns.out).write_text(out, encoding="utf-8")
    else:
        sys.stdout.write(out)

    has_diff = bool(res.added or res.removed or res.modified)

    if has_diff:
        return 1
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except SystemExit as e:
        # preserve numeric exit codes
        if isinstance(e.code, int):
            raise
        _eprint(str(e))
        raise SystemExit(2)