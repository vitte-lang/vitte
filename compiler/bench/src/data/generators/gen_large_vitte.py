

#!/usr/bin/env python3
"""gen_invalid.py

Generate invalid/edge-case fixtures for Vitte tooling.

Purpose
- Provide a curated set of *intentionally invalid* inputs for:
  - lexer
  - parser (core + phrase)
  - semantic analysis
  - formatter
  - CLI error paths

Key properties
- Deterministic output (seeded, stable ordering)
- Self-describing index (JSON) + per-category directories
- Safe-by-default (writes via staging + atomic swap)
- Enterprise-friendly: explicit exit codes, verbose logs

Outputs (under --out):
  invalid/
    index.json
    README.md
    lexer/...
    parser/...
    phrase/...
    sema/...
    fmt/...
    cli/...

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_invalid.py --out ../generated/invalid --seed 0
  python3 gen_invalid.py --out ../generated/invalid --seed 123 --max-per-cat 50
  python3 gen_invalid.py --out ../generated/invalid --seed 0 --emit-md

Notes
- Fixtures are intended to evolve over time. Keep them stable: prefer adding new
  cases over modifying existing ones.
"""

from __future__ import annotations

import argparse
import hashlib
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple

from vitte_benchgen.util import Logger, dumps_json_stable
from vitte_benchgen.writer import StagingWriter, WriteOptions


# -----------------------------------------------------------------------------
# Types
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Case:
    id: str
    category: str
    file_name: str
    description: str
    content: str
    tags: Tuple[str, ...] = ()


# -----------------------------------------------------------------------------
# Case generation primitives
# -----------------------------------------------------------------------------


def _case_id(seed: str, category: str, name: str) -> str:
    h = hashlib.sha256()
    h.update(seed.encode("utf-8"))
    h.update(b"\n")
    h.update(category.encode("utf-8"))
    h.update(b"\n")
    h.update(name.encode("utf-8"))
    return h.hexdigest()[:16]


def _mk(category: str, name: str, desc: str, content: str, *, seed: str, tags: Sequence[str] = ()) -> Case:
    cid = _case_id(seed, category, name)
    fname = f"{name}.vitte".replace(" ", "_").replace("/", "_")
    return Case(id=cid, category=category, file_name=fname, description=desc, content=content, tags=tuple(tags))


# -----------------------------------------------------------------------------
# Concrete invalid cases
# -----------------------------------------------------------------------------


def build_cases(seed: str) -> List[Case]:
    """Return a deterministic list of invalid cases."""

    cases: List[Case] = []

    # --- lexer --------------------------------------------------------------
    cases += [
        _mk(
            "lexer",
            "unterminated_string",
            "String literal missing closing quote",
            'fn main()\n  let s = "hello\n.end\n',
            seed=seed,
            tags=["lex", "string"],
        ),
        _mk(
            "lexer",
            "invalid_escape",
            "String contains an invalid escape sequence",
            'fn main()\n  let s = "\\q"\n.end\n',
            seed=seed,
            tags=["lex", "escape"],
        ),
        _mk(
            "lexer",
            "nul_byte",
            "Embedded NUL byte (should be rejected or handled safely)",
            "fn main()\n  let x = 1\x00\n.end\n",
            seed=seed,
            tags=["lex", "binary"],
        ),
        _mk(
            "lexer",
            "unterminated_comment",
            "Block comment not closed",
            "/* comment\nfn main()\n  ret 0\n.end\n",
            seed=seed,
            tags=["lex", "comment"],
        ),
        _mk(
            "lexer",
            "bad_utf8",
            "Invalid UTF-8 bytes represented via surrogate escapes (tooling should not crash)",
            "fn main()\n  let s = \"\udcff\"\n.end\n",
            seed=seed,
            tags=["lex", "unicode"],
        ),
    ]

    # --- parser (core) ------------------------------------------------------
    cases += [
        _mk(
            "parser",
            "missing_end",
            "Missing .end to close a block",
            "fn main()\n  let x = 1\n",
            seed=seed,
            tags=["parse", "block"],
        ),
        _mk(
            "parser",
            "dangling_else",
            "Else without matching if",
            "fn main()\n  else\n    ret 0\n  .end\n.end\n",
            seed=seed,
            tags=["parse", "if"],
        ),
        _mk(
            "parser",
            "bad_fn_sig",
            "Malformed function signature",
            "fn (x: i32) -> i32\n  ret x\n.end\n",
            seed=seed,
            tags=["parse", "fn"],
        ),
        _mk(
            "parser",
            "bad_type_expr",
            "Type expression with invalid generics",
            "type T = Vec[\n",
            seed=seed,
            tags=["parse", "type"],
        ),
        _mk(
            "parser",
            "match_missing_arm",
            "Match with missing arm body",
            "fn main()\n  match x\n    0 =>\n  .end\n.end\n",
            seed=seed,
            tags=["parse", "match"],
        ),
    ]

    # --- phrase layer -------------------------------------------------------
    cases += [
        _mk(
            "phrase",
            "bad_when",
            "Phrase 'when' missing condition",
            "prog demo\n  when\n    say \"hi\"\n  .end\n.end\n",
            seed=seed,
            tags=["phrase", "when"],
        ),
        _mk(
            "phrase",
            "loop_missing_end",
            "Phrase 'loop' missing .end",
            "prog demo\n  loop\n    say \"x\"\n.end\n",
            seed=seed,
            tags=["phrase", "loop"],
        ),
    ]

    # --- sema ---------------------------------------------------------------
    cases += [
        _mk(
            "sema",
            "use_undef_var",
            "Use undefined variable",
            "fn main()\n  ret x\n.end\n",
            seed=seed,
            tags=["sema", "name"],
        ),
        _mk(
            "sema",
            "duplicate_symbol",
            "Duplicate symbol in same scope",
            "fn main()\n  let x = 1\n  let x = 2\n  ret x\n.end\n",
            seed=seed,
            tags=["sema", "scope"],
        ),
        _mk(
            "sema",
            "type_mismatch",
            "Assign string to integer",
            "fn main()\n  let x: i32 = \"a\"\n  ret 0\n.end\n",
            seed=seed,
            tags=["sema", "type"],
        ),
    ]

    # --- fmt ----------------------------------------------------------------
    cases += [
        _mk(
            "fmt",
            "huge_indent",
            "Extreme indentation should not blow up formatter",
            "fn main()\n" + (" " * 2000) + "ret 0\n.end\n",
            seed=seed,
            tags=["fmt", "stress"],
        ),
        _mk(
            "fmt",
            "deep_nesting",
            "Deep nesting for formatter recursion/stack tests",
            "fn main()\n"
            + "".join(["  if true\n" for _ in range(200)])
            + "  ret 0\n"
            + "".join(["  .end\n" for _ in range(200)])
            + ".end\n",
            seed=seed,
            tags=["fmt", "stress"],
        ),
    ]

    # --- cli ----------------------------------------------------------------
    cases += [
        _mk(
            "cli",
            "unknown_flag",
            "Unknown CLI flag simulation (content unused; used as marker)",
            "# used by args_matrix: unknown flag\n",
            seed=seed,
            tags=["cli"],
        ),
        _mk(
            "cli",
            "empty_input",
            "Empty file should produce a friendly error",
            "",
            seed=seed,
            tags=["cli"],
        ),
    ]

    return cases


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def emit(out_dir: Path, *, seed: str, max_per_cat: int, emit_md: bool, logger: Logger) -> None:
    cases = build_cases(seed)
    cases.sort(key=lambda c: (c.category, c.id, c.file_name))

    by_cat: Dict[str, List[Case]] = {}
    for c in cases:
        by_cat.setdefault(c.category, []).append(c)

    pruned: List[Case] = []
    for _cat, lst in by_cat.items():
        pruned.extend(lst[:max_per_cat])

    pruned.sort(key=lambda c: (c.category, c.id, c.file_name))

    logger.info("invalid: emit", "cases", len(pruned), "out", str(out_dir))

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        index_files: List[Dict[str, Any]] = []

        for c in pruned:
            rel = f"{c.category}/{c.file_name}"
            w.write_text(rel, c.content)
            index_files.append(
                {
                    "id": c.id,
                    "category": c.category,
                    "path": rel,
                    "description": c.description,
                    "tags": list(c.tags),
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_invalid.py",
            "seed": seed,
            "max_per_cat": max_per_cat,
            "counts": {"total": len(pruned), **{k: len(v[:max_per_cat]) for k, v in by_cat.items()}},
            "files": index_files,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    counts = index.get("counts", {})
    lines: List[str] = []
    lines.append("# Invalid fixtures")
    lines.append("")
    lines.append("This directory contains intentionally invalid fixtures used to test error paths.")
    lines.append("")
    lines.append("## Counts")
    lines.append("")
    for k, v in sorted(counts.items()):
        lines.append(f"- {k}: **{v}**")
    lines.append("")
    lines.append("## Policy")
    lines.append("")
    lines.append("- Prefer adding new cases instead of modifying existing ones.")
    lines.append("- Keep file names stable once published.")
    lines.append("- Keep cases small and focused on a single failure mode.")
    lines.append("")
    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Invalid fixtures index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed', '')}`")
    lines.append(f"- max_per_cat: `{index.get('max_per_cat', 0)}`")
    lines.append("")

    files = index.get("files", [])
    by: Dict[str, List[Dict[str, Any]]] = {}
    for f in files:
        by.setdefault(str(f.get("category", "")), []).append(f)

    for cat in sorted(by.keys()):
        lines.append(f"## {cat}")
        lines.append("")
        lines.append("| id | path | description | tags |")
        lines.append("|---|---|---|---|")
        for f in by[cat]:
            cid = str(f.get("id", ""))
            path = str(f.get("path", ""))
            desc = str(f.get("description", ""))
            tags = ",".join([str(t) for t in f.get("tags", [])])
            lines.append(f"| `{cid}` | `{path}` | {desc} | `{tags}` |")
        lines.append("")

    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_invalid.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--max-per-cat", type=int, default=200, help="Max cases per category")
    ap.add_argument("--emit-md", action="store_true", help="Also emit INDEX.md")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    return ap.parse_args(list(argv))


def main(argv: Optional[List[str]] = None) -> int:
    import sys

    ns = parse_args(argv if argv is not None else sys.argv[1:])

    out_dir = Path(ns.out).expanduser().resolve()
    if ns.max_per_cat <= 0:
        _eprint("error: --max-per-cat must be > 0")
        return 2

    log = Logger(level="debug" if ns.verbose else "info", quiet=bool(ns.quiet))

    try:
        emit(out_dir, seed=str(ns.seed), max_per_cat=int(ns.max_per_cat), emit_md=bool(ns.emit_md), logger=log)
    except Exception as e:
        _eprint(f"error: gen_invalid failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())