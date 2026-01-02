

#!/usr/bin/env python3
"""gen_stats.py

Generate deterministic "stats" reports and baselines for Vitte bench tooling.

This generator is intentionally implementation-agnostic:
- It does not require the Vitte compiler.
- It computes *approximate* source-level statistics using a lightweight tokenizer.

Use-cases
- Bench baselines (bytes/lines/tokens)
- Regression tracking (file counts, top largest inputs)
- CI artifacts (small JSON summaries)

Inputs
- One or more root directories to analyze.
- If --inputs is omitted, defaults to `../generated` relative to this script
  (i.e. sibling of the `generators/` directory).

Outputs (under --out):
  stats/
    index.json
    README.md
    INDEX.md                      (optional)
    reports/
      <id>_<name>/
        summary.json
        by_ext.json
        top_files.json
        meta.json

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_stats.py --out ../generated/stats
  python3 gen_stats.py --out ../generated/stats --inputs ../generated/parse ../generated/sema --emit-md
  python3 gen_stats.py --out ../generated/stats --include-ext vitte,muf --top 100 --max-files 5000
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

from vitte_benchgen.rng import Rng
from vitte_benchgen.util import Logger, dumps_json_stable
from vitte_benchgen.writer import StagingWriter, WriteOptions


# -----------------------------------------------------------------------------
# Tokenization / per-file stats (approx)
# -----------------------------------------------------------------------------


_TOKEN_RE = re.compile(
    r"[A-Za-z_][A-Za-z0-9_]*"  # ident-ish
    r"|0x[0-9A-Fa-f_]+"  # hex
    r"|0b[01_]+"  # bin
    r"|\d+(?:\.\d+)?(?:e[+-]?\d+)?"  # numbers
    r'|\"(?:\\.|[^\"\\])*\"'  # strings
    r"|\S"  # fallback single char
)


def _sha256_bytes(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


@dataclass(frozen=True)
class FileStats:
    relpath: str
    ext: str
    bytes: int
    lines: int
    max_line_len: int
    approx_tokens: int
    sha256: str


def file_stats(path: Path, relpath: str) -> FileStats:
    b = path.read_bytes()
    try:
        s = b.decode("utf-8")
    except UnicodeDecodeError:
        s = b.decode("utf-8", errors="replace")

    lines_list = s.splitlines() or [""]
    toks = _TOKEN_RE.findall(s)

    ext = path.suffix.lower().lstrip(".")
    return FileStats(
        relpath=relpath,
        ext=ext,
        bytes=len(b),
        lines=len(lines_list),
        max_line_len=max((len(x) for x in lines_list), default=0),
        approx_tokens=len(toks),
        sha256=_sha256_bytes(b),
    )


# -----------------------------------------------------------------------------
# Aggregation
# -----------------------------------------------------------------------------


@dataclass
class Agg:
    files: int = 0
    bytes: int = 0
    lines: int = 0
    approx_tokens: int = 0
    max_file_bytes: int = 0
    max_file_lines: int = 0
    max_file_tokens: int = 0

    def add(self, fs: FileStats) -> None:
        self.files += 1
        self.bytes += fs.bytes
        self.lines += fs.lines
        self.approx_tokens += fs.approx_tokens
        self.max_file_bytes = max(self.max_file_bytes, fs.bytes)
        self.max_file_lines = max(self.max_file_lines, fs.lines)
        self.max_file_tokens = max(self.max_file_tokens, fs.approx_tokens)


def _default_excludes() -> List[str]:
    return [
        "**/.git/**",
        "**/.hg/**",
        "**/.svn/**",
        "**/.DS_Store",
        "**/node_modules/**",
        "**/dist/**",
        "**/build/**",
        "**/out/**",
        "**/target/**",
        "**/__pycache__/**",
    ]


def _matches_any_glob(rel: str, patterns: Sequence[str]) -> bool:
    # Path.match expects POSIX-like patterns.
    p = Path(rel)
    for pat in patterns:
        if p.match(pat):
            return True
    return False


def iter_files(
    roots: Sequence[Path],
    *,
    include_ext: Sequence[str],
    exclude_globs: Sequence[str],
    logger: Logger,
) -> Iterable[Tuple[Path, str, str]]:
    # yields: (abs_path, root_name, rel_path)
    inc = set([e.lower().lstrip(".") for e in include_ext if e])

    for root in roots:
        root = root.resolve()
        root_name = root.name
        if not root.exists():
            logger.warn("stats: input missing", "root", str(root))
            continue

        for dirpath, dirnames, filenames in os.walk(root):
            # prune excluded dirs
            dp_rel = str(Path(dirpath).resolve().relative_to(root).as_posix())
            # dirpath itself can be excluded; if so prune children
            if dp_rel == ".":
                dp_rel = ""

            # Mutate dirnames to prune.
            keep_dirs: List[str] = []
            for d in dirnames:
                rel = (Path(dp_rel) / d).as_posix() if dp_rel else d
                if _matches_any_glob(rel + "/", exclude_globs):
                    continue
                keep_dirs.append(d)
            dirnames[:] = keep_dirs

            for fn in filenames:
                rel = (Path(dp_rel) / fn).as_posix() if dp_rel else fn
                if _matches_any_glob(rel, exclude_globs):
                    continue

                ext = Path(fn).suffix.lower().lstrip(".")
                if inc and ext not in inc:
                    continue

                yield (Path(dirpath) / fn, root_name, rel)


def _stable_pick(
    *,
    seed: str,
    items: Sequence[Tuple[Path, str, str]],
    max_files: int,
) -> List[Tuple[Path, str, str]]:
    if max_files <= 0 or max_files >= len(items):
        return list(items)

    # Deterministic sample by hashing (seed + rel path + root name)
    scored: List[Tuple[str, Tuple[Path, str, str]]] = []
    for ap, rn, rel in items:
        h = hashlib.sha256((seed + "\n" + rn + "\n" + rel).encode("utf-8")).hexdigest()
        scored.append((h, (ap, rn, rel)))

    scored.sort(key=lambda t: t[0])
    return [x for _, x in scored[:max_files]]


# -----------------------------------------------------------------------------
# Reports
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Report:
    id: str
    name: str
    description: str
    roots: Tuple[str, ...]
    include_ext: Tuple[str, ...]
    exclude_globs: Tuple[str, ...]
    max_files: int
    top: int


def _report_id(seed: str, name: str) -> str:
    h = hashlib.sha256()
    h.update(seed.encode("utf-8"))
    h.update(b"\n")
    h.update(name.encode("utf-8"))
    return h.hexdigest()[:16]


def make_default_reports(
    seed: str,
    roots: Sequence[Path],
    include_ext: Sequence[str],
    exclude_globs: Sequence[str],
    max_files: int,
    top: int,
) -> List[Report]:
    # One report for all roots together + per-root reports.
    reports: List[Report] = []

    all_name = "all_inputs"
    reports.append(
        Report(
            id=_report_id(seed, all_name),
            name=all_name,
            description="Aggregate stats across all input roots",
            roots=tuple([r.name for r in roots]),
            include_ext=tuple(include_ext),
            exclude_globs=tuple(exclude_globs),
            max_files=max_files,
            top=top,
        )
    )

    for r in roots:
        nm = "root_" + r.name
        reports.append(
            Report(
                id=_report_id(seed, nm),
                name=nm,
                description=f"Stats for root: {r}",
                roots=(r.name,),
                include_ext=tuple(include_ext),
                exclude_globs=tuple(exclude_globs),
                max_files=max_files,
                top=top,
            )
        )

    return reports


def compute_report(
    rep: Report,
    *,
    seed: str,
    roots_map: Dict[str, Path],
    logger: Logger,
) -> Tuple[Dict[str, Any], Dict[str, Any], Dict[str, Any]]:
    # Filter roots for this report
    roots = [roots_map[n] for n in rep.roots if n in roots_map]

    files_list = list(
        iter_files(
            roots,
            include_ext=rep.include_ext,
            exclude_globs=rep.exclude_globs,
            logger=logger,
        )
    )

    files_list = _stable_pick(seed=seed + ":stats", items=files_list, max_files=rep.max_files)

    total = Agg()
    by_ext: Dict[str, Agg] = {}

    # track top files by bytes/lines/tokens
    top_bytes: List[FileStats] = []
    top_lines: List[FileStats] = []
    top_tokens: List[FileStats] = []

    def _push_top(lst: List[FileStats], fs: FileStats, key: str) -> None:
        lst.append(fs)
        if key == "bytes":
            lst.sort(key=lambda x: (x.bytes, x.relpath))
        elif key == "lines":
            lst.sort(key=lambda x: (x.lines, x.relpath))
        else:
            lst.sort(key=lambda x: (x.approx_tokens, x.relpath))
        if len(lst) > rep.top:
            del lst[0 : len(lst) - rep.top]

    for ap, root_name, rel in files_list:
        # prefix with root name to keep uniqueness stable
        relp = f"{root_name}/{rel}" if root_name else rel
        fs = file_stats(ap, relp)

        total.add(fs)
        by_ext.setdefault(fs.ext or "(none)", Agg()).add(fs)

        _push_top(top_bytes, fs, "bytes")
        _push_top(top_lines, fs, "lines")
        _push_top(top_tokens, fs, "tokens")

    summary: Dict[str, Any] = {
        "contract": {"name": "vitte.stats.v1", "version": 1},
        "report": {"id": rep.id, "name": rep.name, "description": rep.description},
        "inputs": [str(r) for r in roots],
        "filters": {
            "include_ext": list(rep.include_ext),
            "exclude_globs": list(rep.exclude_globs),
            "max_files": rep.max_files,
            "top": rep.top,
        },
        "counts": {
            "files": total.files,
            "bytes": total.bytes,
            "lines": total.lines,
            "approx_tokens": total.approx_tokens,
            "max_file_bytes": total.max_file_bytes,
            "max_file_lines": total.max_file_lines,
            "max_file_tokens": total.max_file_tokens,
        },
    }

    by_ext_json: Dict[str, Any] = {
        "contract": {"name": "vitte.stats.by_ext.v1", "version": 1},
        "report": {"id": rep.id, "name": rep.name},
        "by_ext": {
            ext: {
                "files": a.files,
                "bytes": a.bytes,
                "lines": a.lines,
                "approx_tokens": a.approx_tokens,
                "max_file_bytes": a.max_file_bytes,
                "max_file_lines": a.max_file_lines,
                "max_file_tokens": a.max_file_tokens,
            }
            for ext, a in sorted(by_ext.items(), key=lambda t: t[0])
        },
    }

    top_json: Dict[str, Any] = {
        "contract": {"name": "vitte.stats.top_files.v1", "version": 1},
        "report": {"id": rep.id, "name": rep.name},
        "top": {
            "bytes": [fs.__dict__ for fs in sorted(top_bytes, key=lambda x: (-x.bytes, x.relpath))],
            "lines": [fs.__dict__ for fs in sorted(top_lines, key=lambda x: (-x.lines, x.relpath))],
            "tokens": [fs.__dict__ for fs in sorted(top_tokens, key=lambda x: (-x.approx_tokens, x.relpath))],
        },
    }

    return summary, by_ext_json, top_json


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def emit(
    out_dir: Path,
    *,
    seed: str,
    inputs: Sequence[Path],
    include_ext: Sequence[str],
    exclude_globs: Sequence[str],
    max_files: int,
    top: int,
    emit_md: bool,
    logger: Logger,
) -> None:
    roots = [p.expanduser().resolve() for p in inputs]
    roots = [p for p in roots if p.exists()]

    if not roots:
        raise ValueError("no valid --inputs roots (nothing exists)")

    roots_map = {p.name: p for p in roots}

    reports = make_default_reports(
        seed,
        roots,
        include_ext=include_ext,
        exclude_globs=exclude_globs,
        max_files=max_files,
        top=top,
    )

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        index_reports: List[Dict[str, Any]] = []

        for rep in reports:
            base = f"reports/{rep.id}_{rep.name}"
            summary, by_ext_json, top_json = compute_report(rep, seed=seed, roots_map=roots_map, logger=logger)

            w.write_text(f"{base}/summary.json", dumps_json_stable(summary, indent=2) + "\n")
            w.write_text(f"{base}/by_ext.json", dumps_json_stable(by_ext_json, indent=2) + "\n")
            w.write_text(f"{base}/top_files.json", dumps_json_stable(top_json, indent=2) + "\n")

            meta = {
                "id": rep.id,
                "name": rep.name,
                "description": rep.description,
                "roots": list(rep.roots),
                "filters": {
                    "include_ext": list(rep.include_ext),
                    "exclude_globs": list(rep.exclude_globs),
                    "max_files": rep.max_files,
                    "top": rep.top,
                },
            }
            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")

            index_reports.append(
                {
                    "id": rep.id,
                    "name": rep.name,
                    "description": rep.description,
                    "path": base,
                    "roots": list(rep.roots),
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_stats.py",
            "seed": seed,
            "inputs": [str(p) for p in inputs],
            "filters": {
                "include_ext": list(include_ext),
                "exclude_globs": list(exclude_globs),
                "max_files": max_files,
                "top": top,
            },
            "contract": {"name": "vitte.stats.v1", "version": 1},
            "reports": index_reports,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Stats reports")
    lines.append("")
    lines.append("Approximate source-level statistics for generated corpora.")
    lines.append("")

    c = index.get("contract", {})
    lines.append("## Contract")
    lines.append("")
    lines.append(f"- name: `{c.get('name','')}`")
    lines.append(f"- version: `{c.get('version',0)}`")
    lines.append("")

    lines.append("## Reports")
    lines.append("")
    for r in index.get("reports", []):
        lines.append(f"- `{r.get('name','')}` -> `{r.get('path','')}`")

    lines.append("")
    lines.append("## Notes")
    lines.append("")
    lines.append("- Token counts are approximate (regex tokenizer).")
    lines.append("- Intended for trends and baselines, not exact compiler metrics.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Stats reports index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")

    filt = index.get("filters", {})
    lines.append(f"- include_ext: `{','.join([str(x) for x in filt.get('include_ext', [])])}`")
    lines.append(f"- max_files: `{filt.get('max_files', 0)}`")
    lines.append(f"- top: `{filt.get('top', 0)}`")
    lines.append("")

    lines.append("| id | name | path | roots | description |")
    lines.append("|---|---|---|---|---|")
    for r in index.get("reports", []):
        lines.append(
            "| `{}` | `{}` | `{}` | `{}` | {} |".format(
                r.get("id", ""),
                r.get("name", ""),
                r.get("path", ""),
                ",".join([str(x) for x in r.get("roots", [])]),
                r.get("description", ""),
            )
        )

    lines.append("")
    lines.append("Open each report directory to view `summary.json`, `by_ext.json`, and `top_files.json`.")
    lines.append("")

    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_stats.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument(
        "--inputs",
        nargs="*",
        default=None,
        help="Root directories to analyze (default: ../generated relative to this script)",
    )
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument(
        "--include-ext",
        default="vitte,muf,md,json,txt",
        help="Comma-separated extension whitelist (empty = all)",
    )
    ap.add_argument(
        "--exclude",
        action="append",
        default=None,
        help="Exclude glob (repeatable). Defaults include VCS/build dirs.",
    )
    ap.add_argument(
        "--max-files",
        type=int,
        default=0,
        help="If >0, deterministically sample at most this many files per report",
    )
    ap.add_argument(
        "--top",
        type=int,
        default=50,
        help="How many largest files to include per metric",
    )
    ap.add_argument("--emit-md", action="store_true", help="Emit INDEX.md")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    return ap.parse_args(list(argv))


def main(argv: Optional[List[str]] = None) -> int:
    import sys

    ns = parse_args(argv if argv is not None else sys.argv[1:])

    out_dir = Path(ns.out).expanduser().resolve()

    if ns.max_files < 0:
        _eprint("error: --max-files must be >= 0")
        return 2
    if ns.top <= 0:
        _eprint("error: --top must be > 0")
        return 2

    include_ext = [x.strip() for x in str(ns.include_ext).split(",") if x.strip()]

    excludes = list(ns.exclude) if ns.exclude else []
    if not excludes:
        excludes = _default_excludes()
    else:
        # keep defaults + user excludes
        excludes = _uniq_keep(_default_excludes() + excludes)

    log = Logger(level="debug" if ns.verbose else "info", quiet=bool(ns.quiet))

    # Default inputs if omitted
    if ns.inputs is None:
        script_dir = Path(__file__).resolve().parent
        default_root = (script_dir.parent / "generated").resolve()
        inputs = [default_root]
    else:
        inputs = [Path(p) for p in ns.inputs]

    try:
        emit(
            out_dir,
            seed=str(ns.seed),
            inputs=inputs,
            include_ext=include_ext,
            exclude_globs=excludes,
            max_files=int(ns.max_files),
            top=int(ns.top),
            emit_md=bool(ns.emit_md),
            logger=log,
        )
    except Exception as e:
        _eprint(f"error: gen_stats failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


def _uniq_keep(seq):
    seen = set()
    out = []
    for x in seq:
        if x not in seen:
            seen.add(x)
            out.append(x)
    return out


if __name__ == "__main__":
    raise SystemExit(main())