

#!/usr/bin/env python3
"""gen_tokens.py

Generate deterministic *tokenization* fixtures for Vitte.

Goals
- Provide implementation-agnostic lexer/tokenizer fixtures.
- Cover the "nasty" corners that break tokenization:
  - unicode identifiers
  - numeric formats (dec/hex/bin, underscores)
  - string escapes
  - comments (#...)
  - operators and punctuation density
  - mixed whitespace/newlines

The tokenization performed here is a lightweight reference tokenizer intended
for test data generation and baseline comparisons. It is *not* the compiler
lexer. A compiler-side adapter can compare its output with the real lexer.

Outputs (under --out):
  tokens/
    index.json
    README.md
    INDEX.md                       (optional)
    sets/
      <id>_<name>/
        input.vitte
        expected.tokens.json        # reference tokenizer output
        meta.json

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_tokens.py --out ../generated/tokens --seed 0
  python3 gen_tokens.py --out ../generated/tokens --seed 1 --count 50 --stress 2 --emit-md
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
# Reference tokenizer
# -----------------------------------------------------------------------------


# NOTE: Keep patterns conservative and stable.
# - Comments: '#' to end of line
# - Strings: "..." with backslash escapes
# - Ident: unicode letters/underscore followed by letters/digits/_
#   We accept a broad range by using \w with UNICODE and excluding leading digits.
# - Numbers: dec/float + hex/bin with underscores

_RE_WS = re.compile(r"[ \t]+")
_RE_NL = re.compile(r"\r\n|\n|\r")
_RE_COMMENT = re.compile(r"#[^\r\n]*")
_RE_STRING = re.compile(r'\"(?:\\.|[^\"\\])*\"', re.UNICODE)
_RE_HEX = re.compile(r"0x[0-9A-Fa-f_]+")
_RE_BIN = re.compile(r"0b[01_]+")
_RE_NUM = re.compile(r"\d+(?:_\d+)*(?:\.\d+(?:_\d+)*)?(?:[eE][+-]?\d+)?")
_RE_IDENT = re.compile(r"(?!\d)\w+", re.UNICODE)

# Operators/punct: single-char fallback


@dataclass(frozen=True)
class Tok:
    kind: str
    text: str
    line: int  # 1-based
    col: int   # 1-based


def tokenize(src: str) -> List[Tok]:
    toks: List[Tok] = []

    i = 0
    line = 1
    col = 1

    def adv_text(t: str) -> None:
        nonlocal i, line, col
        i += len(t)
        # update line/col
        if "\n" in t or "\r" in t:
            # normalize by scanning; keep stable even for CRLF
            j = 0
            while j < len(t):
                ch = t[j]
                if ch == "\r":
                    # consume optional \n
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
        # Newlines
        m = _RE_NL.match(src, i)
        if m:
            adv_text(m.group(0))
            continue

        # Spaces/tabs
        m = _RE_WS.match(src, i)
        if m:
            adv_text(m.group(0))
            continue

        # Comments
        m = _RE_COMMENT.match(src, i)
        if m:
            # Treat comment as a token (useful for lexer benches)
            text = m.group(0)
            toks.append(Tok("comment", text, line, col))
            adv_text(text)
            continue

        # String
        m = _RE_STRING.match(src, i)
        if m:
            text = m.group(0)
            toks.append(Tok("string", text, line, col))
            adv_text(text)
            continue

        # Numbers
        for (kind, rx) in (("hex", _RE_HEX), ("bin", _RE_BIN), ("number", _RE_NUM)):
            m = rx.match(src, i)
            if m:
                text = m.group(0)
                toks.append(Tok(kind, text, line, col))
                adv_text(text)
                break
        else:
            # Ident
            m = _RE_IDENT.match(src, i)
            if m:
                text = m.group(0)
                # classify keywords for convenience
                kw = {
                    "fn",
                    "module",
                    "use",
                    "type",
                    "struct",
                    "enum",
                    "union",
                    "let",
                    "const",
                    "if",
                    "elif",
                    "else",
                    "while",
                    "for",
                    "match",
                    "break",
                    "continue",
                    "return",
                    "ret",
                    "say",
                    "set",
                    "do",
                    "when",
                    "loop",
                    "true",
                    "false",
                    ".end",
                }
                k = "kw" if text in kw else "ident"
                toks.append(Tok(k, text, line, col))
                adv_text(text)
            else:
                # single character operator/punct
                ch = src[i]
                toks.append(Tok("punct", ch, line, col))
                adv_text(ch)

    return toks


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
    tokens: Tuple[Tok, ...]
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
    toks = tuple(tokenize(input_vitte))
    return Case(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        input_vitte=input_vitte,
        tokens=toks,
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
            name="smoke_min",
            description="Minimal function with .end",
            kind="core",
            input_vitte="fn main()\n  ret 0\n.end\n",
            tags=["smoke"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="whitespace_newlines",
            description="Mixed whitespace and CRLF newlines",
            kind="core",
            input_vitte="fn\tmain()\r\n  let  x=1\r\n  ret\t x\r\n.end\r\n",
            tags=["ws"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="comments_density",
            description="Line comments in many positions",
            kind="core",
            input_vitte=(
                "# header\n"
                "fn main() # after sig\n"
                "  let x = 1 # inline\n"
                "  # inner\n"
                "  ret x\n"
                ".end\n"
            ),
            tags=["comments"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="numbers_formats",
            description="Numbers: dec/float/hex/bin with underscores",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let a = 123_456\n"
                "  let b = 3.1415\n"
                "  let c = 0xDEAD_BEEF\n"
                "  let d = 0b1010_0101\n"
                "  let e = 1e-3\n"
                "  ret a\n"
                ".end\n"
            ),
            tags=["numbers"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="strings_escapes",
            description="Strings with common escapes",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let s = \"hello\\nworld\\t\\\"quote\\\"\\\\\\"\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["strings"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="unicode_identifiers",
            description="Unicode identifiers and literals",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let café = \"café\"\n"
                "  let μ = 1\n"
                "  let ΑΒΓ = 2\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["unicode"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="operators_punct",
            description="Dense operators/punctuation",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let x = (1+2)*3-4/5%6\n"
                "  if x>=0 && x<=10 || x==42\n"
                "    ret x\n"
                "  .end\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["ops"],
        )
    )

    return cases


# -----------------------------------------------------------------------------
# Random cases
# -----------------------------------------------------------------------------


def _gen_ident(rng: Rng, n: int) -> str:
    alpha = "abcdefghijklmnopqrstuvwxyz"
    return "".join(alpha[rng.randint(0, len(alpha) - 1)] for _ in range(n))


def _rand_string(rng: Rng, n: int) -> str:
    # Keep it lexer-friendly; include escapes with some probability.
    chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-"
    out: List[str] = []
    for _ in range(n):
        if rng.random() < 0.06:
            out.append("\\n")
        elif rng.random() < 0.04:
            out.append("\\t")
        elif rng.random() < 0.03:
            out.append("\\\\")
        elif rng.random() < 0.03:
            out.append("\\\"")
        else:
            out.append(chars[rng.randint(0, len(chars) - 1)])
    return "\"" + "".join(out) + "\""


def _rand_number(rng: Rng) -> str:
    k = rng.randint(0, 4)
    if k == 0:
        return str(rng.randint(0, 10_000))
    if k == 1:
        return f"{rng.randint(0, 999)}_{rng.randint(0, 999):03d}"
    if k == 2:
        return "0x" + "".join("0123456789abcdef"[rng.randint(0, 15)] for _ in range(8))
    if k == 3:
        return "0b" + "".join("01"[rng.randint(0, 1)] for _ in range(12))
    return f"{rng.randint(0, 99)}.{rng.randint(0, 9999):04d}"


def random_cases(seed: str, *, count: int, stress: int, logger: Logger) -> List[Case]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":tokens:random")
    out: List[Case] = []

    for i in range(count):
        name = f"rand_{i:04d}"

        # generate a function with many token kinds
        stmts = 10 + stress * 40
        vars_ = [f"v{j}_{_gen_ident(rng, 4)}" for j in range(3 + stress)]

        lines: List[str] = []
        if rng.random() < 0.25:
            lines.append("# random header")
        lines.append("fn main()")
        for v in vars_:
            if rng.random() < 0.35:
                lines.append(f"  let {v} = {_rand_string(rng, 10 + stress * 6)}")
            else:
                lines.append(f"  let {v} = {_rand_number(rng)}")

        for _ in range(stmts):
            v = rng.choice(vars_)
            if rng.random() < 0.15:
                lines.append(f"  # comment {_gen_ident(rng, 10)}")
                continue

            if rng.random() < 0.25:
                # if with operator soup
                a = rng.choice(vars_)
                b = rng.choice(vars_)
                lines.append(f"  if {a} >= {b} && {a} != 0")
                lines.append(f"    set {v} = {v} + 1")
                lines.append("  .end")
            else:
                a = rng.choice(vars_)
                op = rng.choice(["+", "-", "*", "/", "%", "==", "!=", ">=", "<="])
                rhs = rng.choice([rng.choice(vars_), _rand_number(rng)])
                lines.append(f"  set {v} = {a} {op} {rhs}")

        lines.append(f"  ret {rng.choice(vars_)}")
        lines.append(".end")

        src = "\n".join(lines) + "\n"

        out.append(
            _mk_case(
                seed=seed,
                name=name,
                description=f"Random token fixture {i} (stress={stress})",
                kind="core",
                input_vitte=src,
                tags=["random", "stress" if stress else "random"],
            )
        )

    logger.debug("tokens: random", "count", len(out), "stress", stress)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(c: Case) -> str:
    name = c.name.replace(" ", "_").replace("/", "_")
    return f"{c.id}_{name}"


def _case_contract(c: Case) -> Dict[str, Any]:
    return {
        "contract": {"name": "vitte.tokens.v1", "version": 1},
        "case": {"id": c.id, "name": c.name, "kind": c.kind},
        "description": c.description,
        "counts": {
            "tokens": len(c.tokens),
            "idents": sum(1 for t in c.tokens if t.kind == "ident"),
            "keywords": sum(1 for t in c.tokens if t.kind == "kw"),
            "numbers": sum(1 for t in c.tokens if t.kind in ("number", "hex", "bin")),
            "strings": sum(1 for t in c.tokens if t.kind == "string"),
            "comments": sum(1 for t in c.tokens if t.kind == "comment"),
            "punct": sum(1 for t in c.tokens if t.kind == "punct"),
        },
        "tokens": [t.__dict__ for t in c.tokens],
    }


def emit(out_dir: Path, *, seed: str, count: int, stress: int, emit_md: bool, logger: Logger) -> None:
    curated = curated_cases(seed)
    rnd = random_cases(seed, count=count, stress=stress, logger=logger)

    cases = curated + rnd

    # Stable ordering: curated first then random
    cases.sort(key=lambda c: ("random" in c.tags, c.name, c.id))

    logger.info("tokens: emit", "curated", len(curated), "random", len(rnd), "total", len(cases), "out", str(out_dir))

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        idx: List[Dict[str, Any]] = []

        for c in cases:
            d = _safe_dir_name(c)
            base = f"sets/{d}"

            w.write_text(f"{base}/input.vitte", c.input_vitte)
            w.write_text(f"{base}/expected.tokens.json", dumps_json_stable(_case_contract(c), indent=2) + "\n")

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
            "tool": "gen_tokens.py",
            "seed": seed,
            "count": count,
            "stress": stress,
            "contract": {"name": "vitte.tokens.v1", "version": 1},
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
    lines.append("# Token fixtures")
    lines.append("")
    lines.append("Reference tokenizer fixtures for Vitte. Output is implementation-agnostic.")
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
    lines.append("- This tokenizer is a stable baseline (regex-based), not the compiler lexer.")
    lines.append("- Use an adapter to compare real lexer output to `expected.tokens.json`.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Token fixtures index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- stress: `{index.get('stress',0)}`")
    lines.append("")

    lines.append("| id | name | kind | path | tokens | idents | kw | nums | strings | comments | punct | tags |")
    lines.append("|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|")
    for s in index.get("sets", []):
        c = s.get("counts", {})
        lines.append(
            "| `{}` | `{}` | `{}` | `{}` | {} | {} | {} | {} | {} | {} | {} | `{}` |".format(
                s.get("id", ""),
                s.get("name", ""),
                s.get("kind", ""),
                s.get("path", ""),
                int(c.get("tokens", 0)),
                int(c.get("idents", 0)),
                int(c.get("keywords", 0)),
                int(c.get("numbers", 0)),
                int(c.get("strings", 0)),
                int(c.get("comments", 0)),
                int(c.get("punct", 0)),
                ",".join([str(t) for t in s.get("tags", [])]),
            )
        )

    lines.append("")
    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_tokens.py")
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
        _eprint(f"error: gen_tokens failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())