

"""vitte_benchgen.writer

High-reliability filesystem writer utilities for Vitte generators.

Why this exists:
- Generators produce large trees with many files.
- We want deterministic output and CI-friendly behavior.
- We want to avoid partially-written outputs.

Features:
- Atomic writes (temp + replace)
- Stable normalization of text newlines (LF)
- Optional trailing-whitespace stripping
- Directory staging via a temporary tree and atomic swap
- Manifest generation helpers (sha256 + bytes)
- Simple copy/update logic

This module is stdlib-only.
"""

from __future__ import annotations

import os
import shutil
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, Iterator, List, Optional, Sequence, Tuple

from .util import (
    Logger,
    atomic_write_bytes,
    atomic_write_text,
    ensure_dir,
    is_probably_binary,
    normalize_lf_bytes,
    normalize_lf_text,
    rel_posix,
    sha256_bytes,
    sha256_file,
    stable_walk,
    strip_trailing_whitespace_lines,
)


@dataclass(frozen=True)
class WriteOptions:
    """Options for writing generated files."""

    normalize_lf: bool = True
    strip_trailing_ws: bool = False
    # if True, will error when attempting to write outside root
    enforce_root: bool = True


@dataclass
class WriteStats:
    files_written: int = 0
    bytes_written: int = 0
    dirs_created: int = 0


class TreeWriter:
    """Writer for a generated tree rooted at `root`."""

    def __init__(self, root: Path, *, logger: Optional[Logger] = None, opts: Optional[WriteOptions] = None):
        self.root = root
        self.logger = logger if logger is not None else Logger(quiet=True)
        self.opts = opts if opts is not None else WriteOptions()

    # ---------------------------------------------------------------------
    # path helpers
    # ---------------------------------------------------------------------

    def _resolve_under_root(self, rel: str) -> Path:
        rel = rel.replace("\\", "/")
        # No absolute paths.
        if rel.startswith("/"):
            raise ValueError(f"absolute path not allowed: {rel}")

        p = (self.root / rel).resolve()
        if self.opts.enforce_root:
            rr = self.root.resolve().as_posix().rstrip("/") + "/"
            rp = p.as_posix()
            if not rp.startswith(rr):
                raise ValueError(f"path escapes root: {rel}")
        return p

    def ensure_parent(self, rel: str) -> Path:
        p = self._resolve_under_root(rel)
        parent = p.parent
        if not parent.exists():
            parent.mkdir(parents=True, exist_ok=True)
        return p

    # ---------------------------------------------------------------------
    # primitive writes
    # ---------------------------------------------------------------------

    def write_bytes(self, rel: str, data: bytes) -> None:
        p = self.ensure_parent(rel)
        atomic_write_bytes(p, data)
        self.logger.debug("write-bytes", rel, len(data))

    def write_text(self, rel: str, text: str, *, encoding: str = "utf-8") -> None:
        if self.opts.normalize_lf:
            text = normalize_lf_text(text)
        if self.opts.strip_trailing_ws:
            text = strip_trailing_whitespace_lines(text)

        p = self.ensure_parent(rel)
        atomic_write_text(p, text, encoding=encoding, newline="\n")
        self.logger.debug("write-text", rel, len(text))

    def write_auto(self, rel: str, data: bytes, *, encoding: str = "utf-8") -> None:
        """Write bytes, auto-normalizing LF if it looks like text."""

        if self.opts.normalize_lf and not is_probably_binary(data):
            data = normalize_lf_bytes(data, encoding=encoding, errors="replace")
        self.write_bytes(rel, data)

    # ---------------------------------------------------------------------
    # tree operations
    # ---------------------------------------------------------------------

    def copy_from(self, src_root: Path, *, include: Optional[Sequence[str]] = None, exclude: Optional[Sequence[str]] = None) -> None:
        """Copy files from src_root into this writer root.

        include/exclude are glob patterns on relative posix paths.
        """

        inc = list(include) if include else []
        exc = list(exclude) if exclude else []

        src_root = src_root.resolve()
        for p in stable_walk(src_root):
            if not p.is_file() or p.is_symlink():
                continue
            rel = rel_posix(p, src_root)

            if exc and _match_any(rel, exc):
                continue
            if inc and not _match_any(rel, inc):
                continue

            dst = self._resolve_under_root(rel)
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(p, dst)
            self.logger.debug("copy", rel)

    # ---------------------------------------------------------------------
    # manifest
    # ---------------------------------------------------------------------

    def build_manifest(self, *, hash_files: bool = True) -> Dict[str, Any]:
        """Return a manifest dict for the current tree."""

        files: List[Dict[str, Any]] = []
        root = self.root.resolve()

        for p in stable_walk(root):
            if not p.is_file() or p.is_symlink():
                continue
            rel = rel_posix(p, root)
            try:
                sz = p.stat().st_size
            except OSError:
                continue

            entry: Dict[str, Any] = {"path": rel, "bytes": int(sz)}
            if hash_files:
                try:
                    entry["sha256"] = sha256_file(p)
                except OSError:
                    entry["sha256"] = ""
            files.append(entry)

        return {"files": files}


# -----------------------------------------------------------------------------
# Staging writer (generate into temp dir then swap)
# -----------------------------------------------------------------------------


class StagingWriter:
    """Stage a full tree generation into a temp dir and atomically replace.

    Common pattern:
      with StagingWriter(out_dir) as w:
        w.writer.write_text(...)
        ...
      # on exit: temp tree is swapped into out_dir

    Behavior:
    - Creates sibling temp directory next to the target.
    - On commit, replaces target with staged directory.
      - If target exists: rename to backup then replace.
    - Cleans up on error.
    """

    def __init__(self, target: Path, *, logger: Optional[Logger] = None, opts: Optional[WriteOptions] = None):
        self.target = target.resolve()
        self.logger = logger if logger is not None else Logger(quiet=True)
        self.opts = opts if opts is not None else WriteOptions()

        self._tmp: Optional[Path] = None
        self.writer: Optional[TreeWriter] = None

    def __enter__(self) -> "StagingWriter":
        parent = self.target.parent
        ensure_dir(parent)

        # Create temp dir in same parent for maximum atomicity.
        self._tmp = Path(
            tempfile.mkdtemp(prefix=self.target.name + ".staging.", dir=str(parent))
        ).resolve()
        self.writer = TreeWriter(self._tmp, logger=self.logger, opts=self.opts)
        self.logger.debug("staging-start", str(self._tmp))
        return self

    def __exit__(self, exc_type, exc, tb) -> bool:
        if exc is not None:
            self._cleanup()
            return False

        try:
            self.commit()
        finally:
            # If commit succeeded, tmp becomes target; still cleanup any lingering.
            self._cleanup(keep_target=True)
        return False

    def _cleanup(self, *, keep_target: bool = False) -> None:
        if self._tmp is None:
            return
        if keep_target:
            # If tmp was moved to target, _tmp path may no longer exist.
            if self._tmp.exists():
                shutil.rmtree(self._tmp, ignore_errors=True)
        else:
            shutil.rmtree(self._tmp, ignore_errors=True)
        self._tmp = None
        self.writer = None

    def commit(self) -> None:
        if self._tmp is None:
            raise RuntimeError("staging not started")

        staged = self._tmp
        target = self.target

        # Ensure staged exists.
        if not staged.exists() or not staged.is_dir():
            raise RuntimeError("staged directory missing")

        # If target exists, move it aside.
        backup: Optional[Path] = None
        if target.exists():
            backup = target.with_name(target.name + ".bak")
            # remove old backup if present
            if backup.exists():
                shutil.rmtree(backup, ignore_errors=True)
            os.replace(target, backup)

        # Now replace.
        os.replace(staged, target)
        self.logger.debug("staging-commit", str(target))

        # cleanup backup
        if backup is not None and backup.exists():
            shutil.rmtree(backup, ignore_errors=True)

        # After os.replace, staged path no longer exists; prevent cleanup from deleting target.
        self._tmp = None


# -----------------------------------------------------------------------------
# Internals
# -----------------------------------------------------------------------------


def _match_any(path_posix: str, globs: Sequence[str]) -> bool:
    import fnmatch

    return any(fnmatch.fnmatch(path_posix, g) for g in globs)


__all__ = [
    "WriteOptions",
    "WriteStats",
    "TreeWriter",
    "StagingWriter",
]