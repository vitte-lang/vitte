

#!/usr/bin/env python3
"""gen_parse.py

Generate parsing fixtures for Vitte (core + phrase).

Goal
- Deterministic corpora for:
  - lexer+parser performance benches
  - parser regression tests (append-only)
  - golden tests via adapters (AST snapshot contracts)

Outputs (under --out):
  parse/
    index.json
    README.md
    INDEX.md                     (optional)
    cases/
      <id>_<name>/
        input.vitte
        expected.ast.json         # contract snapshot (adapter-friendly)
        meta.json

Contract: expected.ast.json
- Cross-implementation, stable properties only.
- `should_parse=true` (this generator emits valid inputs only).
- Includes optional `stats` and `toplevel.counts`.

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_parse.py --out ../generated/parse --seed 0
  python3 gen_parse.py --out ../generated/parse --seed 0 --count 200 --stress 2 --emit-md
  python3 gen_parse.py --out ../generated/parse --kinds core --count 500 --stress 3
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
# Stats (approximate tokenizer for bench metadata)
# -----------------------------------------------------------------------------


_TOKEN_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*|0x[0-9A-Fa-f_]+|0b[01_]+|\d+(?:\.\d+)?(?:e[+-]?\d+)?|\"(?:\\.|[^\"\\])*\"|\S")


def source_stats(src: str) -> Dict[str, Any]:
    b = src.encode("utf-8", errors="replace")
    lines = src.splitlines() or [""]
    toks = _TOKEN_RE.findall(src)
    return {
        "bytes": len(b),
        "lines": len(lines),
        "max_line_len": max((len(x) for x in lines), default=0),
        "approx_tokens": len(toks),
    }


# -----------------------------------------------------------------------------
# AST contract
# -----------------------------------------------------------------------------


def _ast_contract(
    *,
    case_id: str,
    name: str,
    kind: str,
    description: str,
    toplevel_counts: Dict[str, int],
    stats: Optional[Dict[str, Any]],
    notes: str = "",
) -> Dict[str, Any]:
    obj: Dict[str, Any] = {
        "contract": {"name": "vitte.parse.ast.contract", "version": 1},
        "case": {"id": case_id, "name": name, "kind": kind},
        "description": description,
        "should_parse": True,
        "toplevel": {"counts": dict(toplevel_counts)},
        "notes": notes,
        # Placeholder for adapters: keep stable.
        "ast": {"nodes": []},
    }
    if stats is not None:
        obj["stats"] = dict(stats)
    return obj


# -----------------------------------------------------------------------------
# Case model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Case:
    id: str
    name: str
    description: str
    kind: str  # core|phrase
    input_vitte: str
    expected: Dict[str, Any]
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
    toplevel_counts: Dict[str, int],
    notes: str,
    tags: Sequence[str] = (),
    with_stats: bool,
) -> Case:
    cid = _case_id(seed, name)
    st = source_stats(input_vitte) if with_stats else None
    exp = _ast_contract(
        case_id=cid,
        name=name,
        kind=kind,
        description=description,
        toplevel_counts=toplevel_counts,
        stats=st,
        notes=notes,
    )
    return Case(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        input_vitte=input_vitte,
        expected=exp,
        tags=tuple(tags),
    )


# -----------------------------------------------------------------------------
# Curated cases
# -----------------------------------------------------------------------------


def curated_cases(seed: str, *, with_stats: bool) -> List[Case]:
    cases: List[Case] = []

    cases.append(
        _mk_case(
            seed=seed,
            name="core_min_fn",
            description="Minimal core function",
            kind="core",
            input_vitte=("fn main()\n" "  ret 0\n" ".end\n"),
            toplevel_counts={"fn": 1},
            notes="Basic smoke for core parser.",
            tags=["core", "smoke"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="core_types",
            description="Core type declarations: struct, enum, type alias",
            kind="core",
            input_vitte=(
                "type Id = i64\n"
                "struct Point\n"
                "  x: i32\n"
                "  y: i32\n"
                ".end\n"
                "enum Color\n"
                "  Red\n"
                "  Green\n"
                "  Blue\n"
                ".end\n"
                "fn main()\n"
                "  let p: Point\n"
                "  ret 0\n"
                ".end\n"
            ),
            toplevel_counts={"type_alias": 1, "struct": 1, "enum": 1, "fn": 1},
            notes="Covers type bodies with .end.",
            tags=["core", "types"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="core_module_use_export",
            description="Module/use/export surface",
            kind="core",
            input_vitte=(
                "module compiler/bench.parse\n"
                "use std/io\n"
                "export *\n"
                "fn main()\n"
                "  ret 0\n"
                ".end\n"
            ),
            toplevel_counts={"module": 1, "use": 1, "export": 1, "fn": 1},
            notes="Tests module path lexing with '/' and '.' segments.",
            tags=["core", "module"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="core_control_flow",
            description="Core control flow constructs",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let sum = 0\n"
                "  for i in 0..10\n"
                "    set sum = sum + i\n"
                "  .end\n"
                "  if sum > 10\n"
                "    set sum = sum - 1\n"
                "  elif sum == 10\n"
                "    set sum = sum + 1\n"
                "  else\n"
                "    set sum = 0\n"
                "  .end\n"
                "  while sum < 100\n"
                "    set sum = sum + 10\n"
                "  .end\n"
                "  ret sum\n"
                ".end\n"
            ),
            toplevel_counts={"fn": 1},
            notes="Exercises statement parsing and nested blocks.",
            tags=["core", "flow"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="core_calls_index_dot",
            description="Calls, indexing, field access chaining",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let x = foo(1, 2).bar[0].baz()\n"
                "  ret x\n"
                ".end\n"
            ),
            toplevel_counts={"fn": 1},
            notes="Covers postfix parsing and chaining.",
            tags=["core", "expr"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="core_literals_unicode",
            description="Strings, escapes, unicode identifiers",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let café = \"a\\n\\t\\\"b\"\n"
                "  let π = 3.141592\n"
                "  ret 0\n"
                ".end\n"
            ),
            toplevel_counts={"fn": 1},
            notes="Exercises UTF-8 identifiers and string escapes.",
            tags=["core", "unicode", "string"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="core_match_patterns",
            description="Match with literal and wildcard patterns",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let x = 2\n"
                "  match x\n"
                "    0 => ret 0\n"
                "    1 => ret 1\n"
                "    _ => ret 9\n"
                "  .end\n"
                "  ret 0\n"
                ".end\n"
            ),
            toplevel_counts={"fn": 1},
            notes="Covers match arms and patterns.",
            tags=["core", "match"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="core_scenario",
            description="Scenario declaration (core)",
            kind="core",
            input_vitte=("scenario smoke()\n" "  ret\n" ".end\n"),
            toplevel_counts={"scenario": 1},
            notes="Scenario syntax smoke.",
            tags=["core", "scenario"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="phrase_prog_basic",
            description="Phrase program with say/do/ret",
            kind="phrase",
            input_vitte=(
                "prog demo\n"
                "  say \"hello\"\n"
                "  do run\n"
                "  ret\n"
                ".end\n"
            ),
            toplevel_counts={"prog": 1},
            notes="Phrase parser smoke.",
            tags=["phrase", "smoke"],
            with_stats=with_stats,
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="phrase_when_loop",
            description="Phrase when + loop nesting",
            kind="phrase",
            input_vitte=(
                "prog demo\n"
                "  when true\n"
                "    say \"ok\"\n"
                "  .end\n"
                "  loop\n"
                "    say \"tick\"\n"
                "    break\n"
                "  .end\n"
                "  ret\n"
                ".end\n"
            ),
            toplevel_counts={"prog": 1},
            notes="Covers phrase block nesting and break.",
            tags=["phrase", "flow"],
            with_stats=with_stats,
        )
    )

    return cases


# -----------------------------------------------------------------------------
# Stress/random cases
# -----------------------------------------------------------------------------


def _gen_ident(rng: Rng, n: int) -> str:
    alpha = "abcdefghijklmnopqrstuvwxyz"
    return "".join(alpha[rng.randint(0, len(alpha) - 1)] for _ in range(n))


def _gen_expr(rng: Rng, depth: int) -> str:
    if depth <= 0:
        k = rng.randint(0, 4)
        if k == 0:
            return str(rng.randint(-1000, 1000))
        if k == 1:
            return "true" if (rng.next_u32() & 1) else "false"
        if k == 2:
            return _gen_ident(rng, rng.randint(1, 8))
        if k == 3:
            return "\"x\""
        return "0"

    op = rng.choice(["+", "-", "*", "/", "<", ">", "==", "&&", "||"])
    a = _gen_expr(rng, depth - 1)
    b = _gen_expr(rng, depth - 1)
    return f"({a} {op} {b})"


def random_core_case(rng: Rng, *, stress: int) -> str:
    expr_depth = 2 + stress
    stmt_count = 10 + stress * 25
    nesting = 2 + stress * 6

    lines: List[str] = []
    lines.append("fn main()")

    vn = 3 + (stress * 2)
    vars_ = [f"v{j}_{_gen_ident(rng, 4)}" for j in range(vn)]
    for v in vars_:
        lines.append(f"  let {v} = {rng.randint(-10, 10)}")

    for _ in range(stmt_count):
        t = rng.choice(vars_)
        lines.append(f"  set {t} = {_gen_expr(rng, expr_depth)}")

    for _ in range(nesting):
        lines.append(f"  if {_gen_expr(rng, 1 + stress // 2)}")

    lines.append("    ret 0")

    for _ in range(nesting):
        lines.append("  .end")

    lines.append(".end")
    return "\n".join(lines) + "\n"


def random_phrase_case(rng: Rng, *, stress: int) -> str:
    lines = ["prog demo"]

    for _ in range(1 + stress):
        lines.append(f"  when {_gen_expr(rng, 1)}")
        lines.append("    say \"ok\"")
        lines.append("  .end")

    for _ in range(1 + stress):
        lines.append("  loop")
        for _k in range(3 + stress):
            lines.append("    say \"tick\"")
        lines.append("    break")
        lines.append("  .end")

    lines.append("  ret")
    lines.append(".end")

    return "\n".join(lines) + "\n"


def random_cases(seed: str, *, count: int, stress: int, kind: str, logger: Logger, with_stats: bool) -> List[Case]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":parse:" + kind)

    out: List[Case] = []
    for i in range(count):
        name = f"rand_{kind}_{i:04d}"
        cid = _case_id(seed, name)

        if kind == "core":
            src = random_core_case(rng, stress=stress)
            counts = {"fn": 1}
            desc = f"Random core parse case {i}"
        else:
            src = random_phrase_case(rng, stress=stress)
            counts = {"prog": 1}
            desc = f"Random phrase parse case {i}"

        exp = _ast_contract(
            case_id=cid,
            name=name,
            kind=kind,
            description=desc,
            toplevel_counts=counts,
            stats=source_stats(src) if with_stats else None,
            notes="Stress parse only; adapter may capture deeper structure.",
        )

        out.append(
            Case(
                id=cid,
                name=name,
                description=desc,
                kind=kind,
                input_vitte=src,
                expected=exp,
                tags=("random", kind, "stress"),
            )
        )

    logger.debug("parse: random", "kind", kind, "count", len(out), "stress", stress)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(case: Case) -> str:
    name = case.name.replace(" ", "_").replace("/", "_")
    return f"{case.id}_{name}"


def emit(out_dir: Path, *, seed: str, kinds: str, count: int, stress: int, emit_md: bool, with_stats: bool, logger: Logger) -> None:
    curated = curated_cases(seed, with_stats=with_stats)

    rnd_core: List[Case] = []
    rnd_phrase: List[Case] = []

    if kinds in ("core", "both"):
        rnd_core = random_cases(seed, count=count, stress=stress, kind="core", logger=logger, with_stats=with_stats)
    if kinds in ("phrase", "both"):
        rnd_phrase = random_cases(seed, count=count, stress=stress, kind="phrase", logger=logger, with_stats=with_stats)

    cases = curated + rnd_core + rnd_phrase

    # Stable ordering: curated first, then random by kind/name.
    cases.sort(key=lambda c: ("random" in c.tags, c.kind, c.name, c.id))

    logger.info(
        "parse: emit",
        "curated",
        len(curated),
        "random",
        len(rnd_core) + len(rnd_phrase),
        "total",
        len(cases),
        "out",
        str(out_dir),
    )

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        index_cases: List[Dict[str, Any]] = []

        for c in cases:
            d = _safe_dir_name(c)
            base = f"cases/{d}"

            w.write_text(f"{base}/input.vitte", c.input_vitte)
            w.write_text(f"{base}/expected.ast.json", dumps_json_stable(c.expected, indent=2) + "\n")

            meta: Dict[str, Any] = {
                "id": c.id,
                "name": c.name,
                "kind": c.kind,
                "description": c.description,
                "tags": list(c.tags),
            }
            if with_stats:
                meta["stats"] = source_stats(c.input_vitte)

            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")

            index_cases.append(
                {
                    "id": c.id,
                    "name": c.name,
                    "kind": c.kind,
                    "path": base,
                    "tags": list(c.tags),
                    "description": c.description,
                    **({"stats": source_stats(c.input_vitte)} if with_stats else {}),
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_parse.py",
            "seed": seed,
            "kinds": kinds,
            "count": count,
            "stress": stress,
            "with_stats": bool(with_stats),
            "contract": {"name": "vitte.parse.ast.contract", "version": 1},
            "counts": {
                "total": len(cases),
                "curated": len(curated),
                "random_core": len(rnd_core),
                "random_phrase": len(rnd_phrase),
            },
            "cases": index_cases,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Parse fixtures")
    lines.append("")
    lines.append("Fixtures for Vitte parsing (core + phrase).")
    lines.append("")

    counts = index.get("counts", {})
    lines.append("## Counts")
    lines.append("")
    for k, v in sorted(counts.items()):
        lines.append(f"- {k}: **{v}**")

    lines.append("")
    lines.append("## Contract")
    lines.append("")
    c = index.get("contract", {})
    lines.append(f"- name: `{c.get('name','')}`")
    lines.append(f"- version: `{c.get('version',0)}`")
    lines.append("")

    lines.append("## Policy")
    lines.append("")
    lines.append("- Prefer adding new cases instead of modifying existing ones.")
    lines.append("- Keep case directory names stable once published.")
    lines.append("- Expected outputs are a contract; adapt your internal AST in tests.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Parse fixtures index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- kinds: `{index.get('kinds','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- stress: `{index.get('stress',0)}`")
    lines.append(f"- with_stats: `{index.get('with_stats', False)}`")
    lines.append("")

    has_stats = bool(index.get("with_stats", False))

    if has_stats:
        lines.append("| id | kind | name | path | tokens | bytes | lines | tags | description |")
        lines.append("|---|---|---|---|---:|---:|---:|---|---|")
        for c in index.get("cases", []):
            st = c.get("stats", {})
            lines.append(
                "| `{}` | `{}` | `{}` | `{}` | {} | {} | {} | `{}` | {} |".format(
                    c.get("id", ""),
                    c.get("kind", ""),
                    c.get("name", ""),
                    c.get("path", ""),
                    int(st.get("approx_tokens", 0)),
                    int(st.get("bytes", 0)),
                    int(st.get("lines", 0)),
                    ",".join([str(t) for t in c.get("tags", [])]),
                    c.get("description", ""),
                )
            )
    else:
        lines.append("| id | kind | name | path | tags | description |")
        lines.append("|---|---|---|---|---|---|")
        for c in index.get("cases", []):
            lines.append(
                "| `{}` | `{}` | `{}` | `{}` | `{}` | {} |".format(
                    c.get("id", ""),
                    c.get("kind", ""),
                    c.get("name", ""),
                    c.get("path", ""),
                    ",".join([str(t) for t in c.get("tags", [])]),
                    c.get("description", ""),
                )
            )

    lines.append("")
    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_parse.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--kinds", choices=["core", "phrase", "both"], default="both", help="Which languages to generate")
    ap.add_argument("--count", type=int, default=0, help="Random cases per kind")
    ap.add_argument("--stress", type=int, default=0, help="Stress level (0..n)")
    ap.add_argument("--emit-md", action="store_true", help="Emit INDEX.md")
    ap.add_argument("--no-stats", action="store_true", help="Disable stats in meta/expected/index")
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

    with_stats = not bool(ns.no_stats)

    log = Logger(level="debug" if ns.verbose else "info", quiet=bool(ns.quiet))

    try:
        emit(
            out_dir,
            seed=str(ns.seed),
            kinds=str(ns.kinds),
            count=int(ns.count),
            stress=int(ns.stress),
            emit_md=bool(ns.emit_md),
            with_stats=with_stats,
            logger=log,
        )
    except Exception as e:
        _eprint(f"error: gen_parse failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())