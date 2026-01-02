
#!/usr/bin/env python3
"""normalize_lf.py

Normalize newline endings to LF (\n) for text fixtures under a root directory.

Goals (enterprise-grade for CI + tooling):
- Normalize CRLF -> LF for text files.
- Preserve UTF-8 content (decoding errors replaced by U+FFFD by default, optional strict).
- Avoid touching binary files.
- Support dry-run, include/exclude globs, per-file reporting.
- Stable exit codes for automation.

Exit codes:
- 0: success (no fatal errors)
- 2: usage / invalid args
- 3: IO error encountered (some files may still have been processed)

Examples:
  python3 normalize_lf.py --root ../generated
  python3 normalize_lf.py --root ../generated --dry-run --verbose
  python3 normalize_lf.py --root ../generated --include "**/*.vitte" --exclude "**/out/**"
  python3 normalize_lf.py --root ../generated --encoding strict
"""

from __future__ import annotations

import argparse
import fnmatch
import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Optional, Tuple


DEFAULT_TEXT_EXT = {
    ".vitte",
    ".vit",
    ".v",
    ".txt",
    ".md",
    ".json",
    ".csv",
    ".sha256",
    ".yml",
    ".yaml",
    ".toml",
    ".ini",
    ".cfg",
    ".sh",
    ".ps1",
    ".py",
    ".lua",
}


@dataclass
class Stats:
    scanned: int = 0
    eligible: int = 0
    binary_skipped: int = 0
    unchanged: int = 0
    changed: int = 0
    errors: int = 0


def eprint(*a: object) -> None:
    print(*a, file=sys.stderr)


def parse_args(argv: List[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="normalize_lf.py")
    ap.add_argument("--root", required=True, help="Root directory to scan")

    ap.add_argument(
        "--include",
        action="append",
        default=[],
        help="Glob(s) to include (can repeat; comma-separated allowed). Default: by extension.",
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
        help="Additional file extensions to treat as text (e.g. .rs). Can repeat.",
    )

    ap.add_argument(
        "--encoding",
        choices=["replace", "strict"],
        default="replace",
        help="UTF-8 decode strategy when normalizing files",
    )
    ap.add_argument("--dry-run", action="store_true", help="Report changes without writing")
    ap.add_argument("--verbose", action="store_true", help="Print per-file actions")
    ap.add_argument("--quiet", action="store_true", help="Only print summary")

    ap.add_argument(
        "--max-bytes",
        type=int,
        default=64 * 1024 * 1024,
        help="Skip files larger than this many bytes (default: 64 MiB)",
    )

    return ap.parse_args(argv)


def _split_globs(items: List[str]) -> List[str]:
    out: List[str] = []
    for it in items:
        if not it:
            continue
        for part in it.split(","):
            p = part.strip()
            if p:
                out.append(p)
    return out


def _is_binary_bytes(b: bytes) -> bool:
    # Heuristic: if it contains NUL in the first chunk, treat as binary.
    head = b[:4096]
    return b"\x00" in head


def _match_any(path_posix: str, globs: List[str]) -> bool:
    return any(fnmatch.fnmatch(path_posix, g) for g in globs)


def _eligible_by_ext(p: Path, text_ext: set[str]) -> bool:
    return p.suffix.lower() in text_ext


def iter_files(root: Path) -> Iterable[Path]:
    # Deterministic traversal.
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        filenames.sort()
        for fn in filenames:
            yield Path(dirpath) / fn


def normalize_file(
    path: Path,
    *,
    encoding_mode: str,
    dry_run: bool,
    max_bytes: int,
) -> Tuple[bool, bool]:
    """Return (changed, is_binary). Raises OSError on IO failures."""
    b = path.read_bytes()

    if len(b) > max_bytes:
        return (False, False)

    if _is_binary_bytes(b):
        return (False, True)

    if b"\r\n" not in b:
        return (False, False)

    errors = "replace" if encoding_mode == "replace" else "strict"
    s = b.decode("utf-8", errors=errors)
    s = s.replace("\r\n", "\n")

    if dry_run:
        return (True, False)

    # Write with newline='\n' to keep LF, no platform translation.
    path.write_text(s, encoding="utf-8", newline="\n")
    return (True, False)


def main(argv: Optional[List[str]] = None) -> int:
    ns = parse_args(sys.argv[1:] if argv is None else argv)

    root = Path(ns.root).expanduser().resolve()
    if not root.exists() or not root.is_dir():
        eprint(f"error: --root is not a directory: {root}")
        return 2

    include_globs = _split_globs(ns.include)
    exclude_globs = _split_globs(ns.exclude)

    text_ext = set(DEFAULT_TEXT_EXT)
    for e in ns.ext:
        if not e:
            continue
        ext = e.strip()
        if not ext:
            continue
        if not ext.startswith("."):
            ext = "." + ext
        text_ext.add(ext.lower())

    st = Stats()

    root_posix = root.as_posix().rstrip("/") + "/"

    def rel_posix(p: Path) -> str:
        rp = p.resolve().as_posix()
        if rp.startswith(root_posix):
            return rp[len(root_posix) :]
        # Fallback
        return p.as_posix()

    for p in iter_files(root):
        st.scanned += 1

        # ignore symlinks (safe default)
        try:
            if p.is_symlink():
                continue
        except OSError:
            # treat as error but continue
            st.errors += 1
            continue

        rel = rel_posix(p)

        if exclude_globs and _match_any(rel, exclude_globs):
            continue

        eligible = False
        if include_globs:
            eligible = _match_any(rel, include_globs)
        else:
            eligible = _eligible_by_ext(p, text_ext)

        if not eligible:
            continue

        st.eligible += 1

        try:
            changed, is_binary = normalize_file(
                p,
                encoding_mode=ns.encoding,
                dry_run=bool(ns.dry_run),
                max_bytes=int(ns.max_bytes),
            )
        except OSError as e:
            st.errors += 1
            if not ns.quiet:
                eprint(f"io error: {rel}: {e}")
            continue

        if is_binary:
            st.binary_skipped += 1
            if ns.verbose and not ns.quiet:
                print(f"[skip-binary] {rel}")
            continue

        if changed:
            st.changed += 1
            if ns.verbose and not ns.quiet:
                if ns.dry_run:
                    print(f"[would-fix] {rel}")
                else:
                    print(f"[fixed] {rel}")
        else:
            st.unchanged += 1
            if ns.verbose and not ns.quiet:
                print(f"[ok] {rel}")

    # Summary
    if not ns.quiet:
        mode = "dry-run" if ns.dry_run else "write"
        print(
            f"normalize_lf: root={root} mode={mode} scanned={st.scanned} eligible={st.eligible} "
            f"changed={st.changed} unchanged={st.unchanged} binary_skipped={st.binary_skipped} errors={st.errors}"
        )
    else:
        print(f"normalize_lf: changed={st.changed} errors={st.errors}")

    if st.errors:
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
