

#!/usr/bin/env python3
"""gen_trivia.py

Generate deterministic *trivia* fixtures for Vitte.

In compiler/tooling pipelines, "trivia" refers to non-semantic tokens that must
be preserved by the formatter / rewriter:
- whitespace
- newlines
- comments

This generator produces tricky inputs that stress:
- mixed indentation (spaces/tabs)
- CRLF vs LF
- comment placement (inline, trailing, standalone)
- consecutive blank lines
- unicode in comments
- "operator soup" near comments
- very long comment lines

Outputs (under --out):
  trivia/
    index.json
    README.md
    INDEX.md                       (optional)
    sets/
      <id>_<name>/
        input.vitte
        expected.trivia.json       # reference trivia extraction
        meta.json

The reference trivia extraction here is a lightweight baseline. A compiler-side
adapter can compare its own trivia stream to this output.

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_trivia.py --out ../generated/trivia --seed 0
  python3 gen_trivia.py --out ../generated/trivia --seed 0 --count 50 --stress 2 --emit-md
"""

from __future__ import annotations

import argparse
import hashlib
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple

from vitte_benchgen.rng import Rng
from vitte_benchgen.util import Logger, dumps_json_stable
from vitte_benchgen.writer import StagingWriter, WriteOptions


# -----------------------------------------------------------------------------
# Reference trivia extractor
# -----------------------------------------------------------------------------


_RE_NL = re.compile(r"\r\n|\n|\r")
_RE_WS = re.compile(r"[ \t]+")
_RE_COMMENT = re.compile(r"#[^\r\n]*")


@dataclass(frozen=True)
class Trivia:
    kind: str  # ws | nl | comment
    text: str
    line: int  # 1-based start
    col: int   # 1-based start


def extract_trivia(src: str) -> List[Trivia]:
    out: List[Trivia] = []

    i = 0
    line = 1
    col = 1

    def adv_text(t: str) -> None:
        nonlocal i, line, col
        i += len(t)
        if "\n" in t or "\r" in t:
            j = 0
            while j < len(t):
                ch = t[j]
                if ch == "\r":
                    if j + 1 < len(t) and t[j + 1] == "\n":
                        j += 1
                    line += 1
                    col = 1
                elif ch == "\n":
                    line += 1
                    col = 1
                else:
                    col += 1
                j += 1
        else:
            col += len(t)

    while i < len(src):
        m = _RE_NL.match(src, i)
        if m:
            t = m.group(0)
            out.append(Trivia("nl", t, line, col))
            adv_text(t)
            continue

        m = _RE_WS.match(src, i)
        if m:
            t = m.group(0)
            out.append(Trivia("ws", t, line, col))
            adv_text(t)
            continue

        m = _RE_COMMENT.match(src, i)
        if m:
            t = m.group(0)
            out.append(Trivia("comment", t, line, col))
            adv_text(t)
            continue

        # Non-trivia character; advance by 1.
        adv_text(src[i])

    return out


# -----------------------------------------------------------------------------
# Case model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Case:
    id: str
    name: str
    description: str
    kind: str
    input_vitte: str
    trivia: Tuple[Trivia, ...]
    tags: Tuple[str, ...] = ()


def _case_id(seed: str, name: str) -> str:
    h = hashlib.sha256()
    h.update(seed.encode("utf-8"))
    h.update(b"\n")
    h.update(name.encode("utf-8"))
    return h.hexdigest()[:16]


def _mk_case(
    *,
    seed: str,
    name: str,
    description: str,
    kind: str,
    input_vitte: str,
    tags: Sequence[str] = (),
) -> Case:
    cid = _case_id(seed, name)
    triv = tuple(extract_trivia(input_vitte))
    return Case(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        input_vitte=input_vitte,
        trivia=triv,
        tags=tuple(tags),
    )


# -----------------------------------------------------------------------------
# Curated cases
# -----------------------------------------------------------------------------


def curated_cases(seed: str) -> List[Case]:
    cases: List[Case] = []

    cases.append(
        _mk_case(
            seed=seed,
            name="trailing_inline_comments",
            description="Inline and trailing comments around operators",
            kind="core",
            input_vitte=(
                "fn main()  # sig\n"
                "  let x = 1 + 2 # add\n"
                "  let y = x*3  # mul\n"
                "  ret y # done\n"
                ".end\n"
            ),
            tags=["comments"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="blank_lines_blocks",
            description="Consecutive blank lines inside blocks",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "\n"
                "  let x = 0\n"
                "\n"
                "\n"
                "  if true\n"
                "\n"
                "    set x = x + 1\n"
                "\n"
                "  .end\n"
                "\n"
                "  ret x\n"
                ".end\n"
            ),
            tags=["blank", "blocks"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="tabs_spaces_mix",
            description="Mixed indentation: tabs + spaces",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "\tlet x = 1\n"
                "\tif true\n"
                "\t  set x = x + 1\n"
                "\t.end\n"
                "\tret x\n"
                ".end\n"
            ),
            tags=["tabs", "indent"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="crlf_newlines",
            description="CRLF newlines with comments",
            kind="core",
            input_vitte="fn main()\r\n  # c1\r\n  ret 0\r\n.end\r\n",
            tags=["crlf"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="unicode_comments",
            description="Unicode content in comments",
            kind="core",
            input_vitte=(
                "# café μ 漢字\n"
                "fn main()\n"
                "  # Привет\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["unicode", "comments"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="very_long_comment",
            description="Very long comment line (stress)",
            kind="core",
            input_vitte=(
                "fn main()\n"
                f"  # {'x' * 4000}\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["stress", "comments"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="operator_soup_with_comments",
            description="Operators near comments",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let x = (1+2)*3-4/5%6 # math\n"
                "  if x>=0 && x<=10 || x==42 # cond\n"
                "    ret x\n"
                "  .end\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["ops", "comments"],
        )
    )

    return cases


# -----------------------------------------------------------------------------
# Random cases
# -----------------------------------------------------------------------------


def _gen_ident(rng: Rng, n: int) -> str:
    alpha = "abcdefghijklmnopqrstuvwxyz"
    return "".join(alpha[rng.randint(0, len(alpha) - 1)] for _ in range(n))


def random_cases(seed: str, *, count: int, stress: int, logger: Logger) -> List[Case]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":trivia:random")
    out: List[Case] = []

    for i in range(count):
        name = f"rand_{i:04d}"

        lines: List[str] = []
        if rng.random() < 0.25:
            lines.append("# header " + _gen_ident(rng, 10))

        lines.append("fn main()")

        # mix indentation patterns
        base_indent = "\t" if rng.random() < 0.35 else "  "

        stmt_n = 10 + stress * 40
        for _ in range(stmt_n):
            if rng.random() < 0.12:
                lines.append("")  # blank
                continue

            if rng.random() < 0.20:
                # standalone comment
                lines.append(base_indent + "# " + _gen_ident(rng, 20 + stress * 10))
                continue

            if rng.random() < 0.25:
                # inline comment
                lines.append(base_indent + "let x = 1 + 2 # " + _gen_ident(rng, 12))
            else:
                lines.append(base_indent + "set x = x + 1")

        lines.append(base_indent + "ret 0")
        lines.append(".end")

        # newline mode
        if rng.random() < 0.20:
            src = "\r\n".join(lines) + "\r\n"
        else:
            src = "\n".join(lines) + "\n"

        out.append(
            _mk_case(
                seed=seed,
                name=name,
                description=f"Random trivia fixture {i} (stress={stress})",
                kind="core",
                input_vitte=src,
                tags=["random", "stress" if stress else "random"],
            )
        )

    logger.debug("trivia: random", "count", len(out), "stress", stress)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(c: Case) -> str:
    name = c.name.replace(" ", "_").replace("/", "_")
    return f"{c.id}_{name}"


def _case_contract(c: Case) -> Dict[str, Any]:
    return {
        "contract": {"name": "vitte.trivia.v1", "version": 1},
        "case": {"id": c.id, "name": c.name, "kind": c.kind},
        "description": c.description,
        "counts": {
            "trivia": len(c.trivia),
            "ws": sum(1 for t in c.trivia if t.kind == "ws"),
            "nl": sum(1 for t in c.trivia if t.kind == "nl"),
            "comments": sum(1 for t in c.trivia if t.kind == "comment"),
        },
        "trivia": [t.__dict__ for t in c.trivia],
    }


def emit(out_dir: Path, *, seed: str, count: int, stress: int, emit_md: bool, logger: Logger) -> None:
    curated = curated_cases(seed)
    rnd = random_cases(seed, count=count, stress=stress, logger=logger)

    cases = curated + rnd

    cases.sort(key=lambda c: ("random" in c.tags, c.name, c.id))

    logger.info("trivia: emit", "curated", len(curated), "random", len(rnd), "total", len(cases), "out", str(out_dir))

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        idx: List[Dict[str, Any]] = []

        for c in cases:
            d = _safe_dir_name(c)
            base = f"sets/{d}"

            w.write_text(f"{base}/input.vitte", c.input_vitte)
            w.write_text(f"{base}/expected.trivia.json", dumps_json_stable(_case_contract(c), indent=2) + "\n")

            meta = {
                "id": c.id,
                "name": c.name,
                "kind": c.kind,
                "description": c.description,
                "tags": list(c.tags),
            }
            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")

            idx.append(
                {
                    "id": c.id,
                    "name": c.name,
                    "kind": c.kind,
                    "path": base,
                    "tags": list(c.tags),
                    "description": c.description,
                    "counts": _case_contract(c)["counts"],
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_trivia.py",
            "seed": seed,
            "count": count,
            "stress": stress,
            "contract": {"name": "vitte.trivia.v1", "version": 1},
            "counts": {
                "total": len(cases),
                "curated": len(curated),
                "random": len(rnd),
            },
            "sets": idx,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Trivia fixtures")
    lines.append("")
    lines.append("Reference trivia extraction fixtures for Vitte (ws/nl/comments).")
    lines.append("")

    counts = index.get("counts", {})
    lines.append("## Counts")
    lines.append("")
    for k, v in sorted(counts.items()):
        lines.append(f"- {k}: **{v}**")

    lines.append("")
    c = index.get("contract", {})
    lines.append("## Contract")
    lines.append("")
    lines.append(f"- name: `{c.get('name','')}`")
    lines.append(f"- version: `{c.get('version',0)}`")

    lines.append("")
    lines.append("## Notes")
    lines.append("")
    lines.append("- This extractor is a stable baseline, not the compiler trivia stream.")
    lines.append("- Use an adapter to compare real trivia to `expected.trivia.json`.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Trivia fixtures index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- stress: `{index.get('stress',0)}`")
    lines.append("")

    lines.append("| id | name | kind | path | trivia | ws | nl | comments | tags |")
    lines.append("|---|---|---|---|---:|---:|---:|---:|---|")
    for s in index.get("sets", []):
        c = s.get("counts", {})
        lines.append(
            "| `{}` | `{}` | `{}` | `{}` | {} | {} | {} | {} | `{}` |".format(
                s.get("id", ""),
                s.get("name", ""),
                s.get("kind", ""),
                s.get("path", ""),
                int(c.get("trivia", 0)),
                int(c.get("ws", 0)),
                int(c.get("nl", 0)),
                int(c.get("comments", 0)),
                ",".join([str(t) for t in s.get("tags", [])]),
            )
        )

    lines.append("")
    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_trivia.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--count", type=int, default=0, help="Number of random cases")
    ap.add_argument("--stress", type=int, default=0, help="Stress level (0..n)")
    ap.add_argument("--emit-md", action="store_true", help="Emit INDEX.md")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    return ap.parse_args(list(argv))


def main(argv: Optional[List[str]] = None) -> int:
    import sys

    ns = parse_args(argv if argv is not None else sys.argv[1:])

    out_dir = Path(ns.out).expanduser().resolve()

    if ns.count < 0:
        _eprint("error: --count must be >= 0")
        return 2
    if ns.stress < 0:
        _eprint("error: --stress must be >= 0")
        return 2

    log = Logger(level="debug" if ns.verbose else "info", quiet=bool(ns.quiet))

    try:
        emit(out_dir, seed=str(ns.seed), count=int(ns.count), stress=int(ns.stress), emit_md=bool(ns.emit_md), logger=log)
    except Exception as e:
        _eprint(f"error: gen_trivia failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())