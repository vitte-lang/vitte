

"""vitte_benchgen.util

Shared utilities for Vitte corpus/fixture generators.

Design goals:
- Determinism by default (stable traversal, stable JSON serialization)
- CI-friendly ergonomics (clear error messages, safe file writes)
- stdlib-first (no mandatory third-party deps)

This module provides:
- filesystem helpers (stable walk, ensure dir, safe write)
- hashing helpers (sha256)
- JSON helpers (stable dumps, load)
- text helpers (newline normalization, line ending detection)
- simple logging

All helpers are intentionally small and composable.
"""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, Iterator, List, Optional, Sequence, Tuple


# -----------------------------------------------------------------------------
# Logging
# -----------------------------------------------------------------------------


def now_ms() -> int:
    return int(time.time() * 1000)


@dataclass
class Logger:
    """Minimal logger.

    Levels: error < warn < info < debug
    """

    level: str = "info"
    quiet: bool = False

    def _lvl(self, name: str) -> int:
        m = {"error": 0, "warn": 1, "info": 2, "debug": 3}
        return m.get(name, 2)

    def _enabled(self, name: str) -> bool:
        return self._lvl(name) <= self._lvl(self.level)

    def _emit(self, tag: str, *a: object) -> None:
        if self.quiet:
            return
        msg = " ".join(str(x) for x in a)
        print(f"[{tag}] {msg}", file=sys.stderr)

    def error(self, *a: object) -> None:
        if self._enabled("error"):
            self._emit("error", *a)

    def warn(self, *a: object) -> None:
        if self._enabled("warn"):
            self._emit("warn", *a)

    def info(self, *a: object) -> None:
        if self._enabled("info"):
            self._emit("info", *a)

    def debug(self, *a: object) -> None:
        if self._enabled("debug"):
            self._emit("debug", *a)


# -----------------------------------------------------------------------------
# Filesystem
# -----------------------------------------------------------------------------


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def stable_walk(root: Path) -> Iterator[Path]:
    """Deterministic recursive walk (sorted)."""

    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        filenames.sort()
        for fn in filenames:
            yield Path(dirpath) / fn


def rel_posix(path: Path, root: Path) -> str:
    """Compute relative path to root, always using POSIX separators."""

    rp = path.resolve().as_posix()
    rr = root.resolve().as_posix().rstrip("/") + "/"
    if rp.startswith(rr):
        return rp[len(rr) :]
    # fallback: may not be under root
    return path.as_posix().replace("\\", "/")


def copy_tree(src: Path, dst: Path, *, logger: Optional[Logger] = None) -> None:
    """Copy a directory tree deterministically.

    Uses shutil.copy2 per file.
    """

    if logger is None:
        logger = Logger(quiet=True)

    if not src.exists() or not src.is_dir():
        raise FileNotFoundError(f"src tree not found: {src}")

    ensure_dir(dst)

    for p in stable_walk(src):
        if not p.is_file() or p.is_symlink():
            continue
        rel = rel_posix(p, src)
        out = dst / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        logger.debug("copy", rel)
        shutil.copy2(p, out)


def atomic_write_bytes(path: Path, data: bytes) -> None:
    """Atomic write: write to temp file then replace."""

    path.parent.mkdir(parents=True, exist_ok=True)

    # Use same directory to maximize atomicity on POSIX.
    with tempfile.NamedTemporaryFile(delete=False, dir=str(path.parent), prefix=path.name + ".tmp.") as tf:
        tmp = Path(tf.name)
        tf.write(data)
        tf.flush()
        os.fsync(tf.fileno())

    os.replace(tmp, path)


def atomic_write_text(path: Path, text: str, *, encoding: str = "utf-8", newline: str = "\n") -> None:
    """Atomic text write with newline normalization."""

    # Normalize newlines in-memory.
    text = text.replace("\r\n", "\n").replace("\r", "\n")

    # If caller asked for a specific newline, translate.
    if newline != "\n":
        text = text.replace("\n", newline)

    atomic_write_bytes(path, text.encode(encoding))


def read_bytes(path: Path) -> bytes:
    return path.read_bytes()


def read_text(path: Path, *, encoding: str = "utf-8", errors: str = "strict") -> str:
    return path.read_text(encoding=encoding, errors=errors)


# -----------------------------------------------------------------------------
# Hashing
# -----------------------------------------------------------------------------


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path, *, chunk: int = 1024 * 1024) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


# -----------------------------------------------------------------------------
# JSON
# -----------------------------------------------------------------------------


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def dumps_json_stable(obj: Any, *, indent: int = 2) -> str:
    """Deterministic JSON serialization.

    - sort_keys=True
    - ensure_ascii=False (keep unicode)
    - stable separators
    """

    return json.dumps(
        obj,
        ensure_ascii=False,
        sort_keys=True,
        indent=indent,
        separators=(",", ": ") if indent else (",", ":"),
    )


def write_json(path: Path, obj: Any, *, indent: int = 2) -> None:
    atomic_write_text(path, dumps_json_stable(obj, indent=indent) + "\n")


# -----------------------------------------------------------------------------
# Text helpers
# -----------------------------------------------------------------------------


def detect_newline_style(data: bytes) -> str:
    """Return 'lf', 'crlf', 'cr', or 'none'."""

    if b"\r\n" in data:
        return "crlf"
    if b"\r" in data:
        return "cr"
    if b"\n" in data:
        return "lf"
    return "none"


def normalize_lf_text(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n")


def normalize_lf_bytes(data: bytes, *, encoding: str = "utf-8", errors: str = "replace") -> bytes:
    """Normalize CRLF/CR newlines to LF for UTF-8 text bytes.

    If decoding fails in strict mode, caller should pass errors='strict'.
    """

    s = data.decode(encoding, errors=errors)
    s = normalize_lf_text(s)
    return s.encode(encoding)


def is_probably_binary(data: bytes) -> bool:
    """Heuristic: NUL byte in first chunk => binary."""

    return b"\x00" in data[:4096]


def strip_trailing_whitespace_lines(text: str) -> str:
    """Strip trailing whitespace per line, preserve final newline if present."""

    had_nl = text.endswith("\n")
    lines = text.splitlines()
    lines = [ln.rstrip(" \t") for ln in lines]
    out = "\n".join(lines)
    if had_nl:
        out += "\n"
    return out


def clamp_int(x: int, lo: int, hi: int) -> int:
    if x < lo:
        return lo
    if x > hi:
        return hi
    return x


__all__ = [
    "Logger",
    "now_ms",
    "ensure_dir",
    "stable_walk",
    "rel_posix",
    "copy_tree",
    "atomic_write_bytes",
    "atomic_write_text",
    "read_bytes",
    "read_text",
    "sha256_bytes",
    "sha256_file",
    "load_json",
    "dumps_json_stable",
    "write_json",
    "detect_newline_style",
    "normalize_lf_text",
    "normalize_lf_bytes",
    "is_probably_binary",
    "strip_trailing_whitespace_lines",
    "clamp_int",
]