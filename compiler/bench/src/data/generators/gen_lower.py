

#!/usr/bin/env python3
"""gen_lower.py

Generate lowering fixtures for the Vitte compiler pipeline.

Scope
- The "lower" stage typically transforms parsed AST (core/phrase) into a simpler
  IR-friendly form (explicit control flow, desugared constructs, normalized
  expressions, stable symbol names, etc.).

This generator produces a deterministic fixture set that is useful for:
- unit tests of the lowerer (golden expectations)
- fuzz-ish stress fixtures (deep nesting, large expressions)
- regression tests (keep stable; append new cases over editing existing ones)

Directory layout (under --out):
  lower/
    index.json
    README.md
    INDEX.md                (optional)
    cases/
      <id>_<name>/
        input.vitte
        expected.lower.json
        meta.json

Notes
- `expected.lower.json` is designed as an *implementation-agnostic contract*.
  It captures the intent and key invariants of the lowering result.
- If your real lowerer produces a different internal IR, add an adapter in tests
  that converts your IR to this contract (or replace this contract once stable).

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_lower.py --out ../generated/lower --seed 0
  python3 gen_lower.py --out ../generated/lower --seed 123 --count 200 --stress 2 --emit-md
"""

from __future__ import annotations

import argparse
import hashlib
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple

from vitte_benchgen.rng import Rng
from vitte_benchgen.util import Logger, dumps_json_stable
from vitte_benchgen.writer import StagingWriter, WriteOptions


# -----------------------------------------------------------------------------
# Contract: expected.lower.json
# -----------------------------------------------------------------------------


def _contract_skeleton(*, case_id: str, name: str, kind: str, notes: str, invariants: Sequence[str]) -> Dict[str, Any]:
    """Return a stable expected.lower.json skeleton."""

    return {
        "contract": {
            "name": "vitte.lower.contract",
            "version": 1,
        },
        "case": {
            "id": case_id,
            "name": name,
            "kind": kind,
        },
        "notes": notes,
        "invariants": list(invariants),
        # The following fields are placeholders until the real adapter is used.
        # Tests may fill them from actual lowering output.
        "ir": {
            "functions": [],
            "globals": [],
        },
    }


# -----------------------------------------------------------------------------
# Case model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Case:
    id: str
    name: str
    description: str
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
    input_vitte: str,
    kind: str,
    invariants: Sequence[str],
    notes: str,
    tags: Sequence[str] = (),
) -> Case:
    cid = _case_id(seed, name)
    exp = _contract_skeleton(case_id=cid, name=name, kind=kind, notes=notes, invariants=invariants)
    return Case(id=cid, name=name, description=description, input_vitte=input_vitte, expected=exp, tags=tuple(tags))


# -----------------------------------------------------------------------------
# Curated fixtures
# -----------------------------------------------------------------------------


def curated_cases(seed: str) -> List[Case]:
    """Curated set: keep stable ordering; append new cases over time."""

    cases: List[Case] = []

    # Basic expression lowering (temporary values / precedence)
    cases.append(
        _mk_case(
            seed=seed,
            name="expr_precedence",
            description="Expression precedence and temporary introduction",
            input_vitte=(
                "fn main()\n"
                "  let a = 1 + 2 * 3 - 4 / 2\n"
                "  ret a\n"
                ".end\n"
            ),
            kind="expr",
            invariants=[
                "no implicit precedence remains (tree is explicit)",
                "introduce temps for complex expressions if IR requires",
            ],
            notes="Lowerer should preserve arithmetic semantics and evaluation order.",
            tags=["expr", "arith"],
        )
    )

    # If/else => explicit CFG
    cases.append(
        _mk_case(
            seed=seed,
            name="if_else_cfg",
            description="If/else desugaring to explicit blocks",
            input_vitte=(
                "fn main()\n"
                "  let x = 2\n"
                "  if x < 0\n"
                "    ret 0\n"
                "  else\n"
                "    ret 1\n"
                "  .end\n"
                ".end\n"
            ),
            kind="cfg",
            invariants=[
                "condition evaluated once",
                "two successors then/else; explicit join or early returns",
            ],
            notes="Expected IR uses explicit branch + blocks.",
            tags=["cfg", "if"],
        )
    )

    # While loop => header/body/latch
    cases.append(
        _mk_case(
            seed=seed,
            name="while_loop",
            description="While loop normalization",
            input_vitte=(
                "fn main()\n"
                "  let i = 0\n"
                "  while i < 10\n"
                "    set i = i + 1\n"
                "  .end\n"
                "  ret i\n"
                ".end\n"
            ),
            kind="cfg",
            invariants=[
                "loop has explicit header with conditional branch",
                "body executes 0..n times",
                "continue targets header; break targets exit",
            ],
            notes="Lowerer should canonicalize loop shape.",
            tags=["cfg", "while"],
        )
    )

    # Short-circuit boolean
    cases.append(
        _mk_case(
            seed=seed,
            name="short_circuit_and_or",
            description="Short-circuit lowering for &&/||",
            input_vitte=(
                "fn main()\n"
                "  let a = true\n"
                "  let b = false\n"
                "  let x = a && (b || a)\n"
                "  if x\n"
                "    ret 1\n"
                "  else\n"
                "    ret 0\n"
                "  .end\n"
                ".end\n"
            ),
            kind="cfg",
            invariants=[
                "short-circuit semantics preserved",
                "no eager evaluation of RHS",
            ],
            notes="Typical lowering emits conditional branches or select patterns.",
            tags=["cfg", "bool"],
        )
    )

    # Match lowering (decision tree / jump table contract)
    cases.append(
        _mk_case(
            seed=seed,
            name="match_int",
            description="Match on integer; lowering to decision tree",
            input_vitte=(
                "fn main()\n"
                "  let x = 2\n"
                "  match x\n"
                "    0 => ret 0\n"
                "    1 => ret 1\n"
                "    _ => ret 9\n"
                "  .end\n"
                ".end\n"
            ),
            kind="cfg",
            invariants=[
                "scrutinee evaluated once",
                "default arm exists (wildcard)",
            ],
            notes="Implementation may choose chain/jump table; contract asserts semantics.",
            tags=["cfg", "match"],
        )
    )

    # Phrase desugaring marker (if your pipeline supports phrase->core lowering)
    cases.append(
        _mk_case(
            seed=seed,
            name="phrase_when_desugar",
            description="Phrase 'when' desugaring marker",
            input_vitte=(
                "prog demo\n"
                "  when true\n"
                "    say \"ok\"\n"
                "  .end\n"
                ".end\n"
            ),
            kind="phrase",
            invariants=[
                "phrase constructs fully desugared to core before IR lowering",
            ],
            notes="If you do not implement phrase, keep as no-op test or skip.",
            tags=["phrase"],
        )
    )

    return cases


# -----------------------------------------------------------------------------
# Stress/random fixtures
# -----------------------------------------------------------------------------


def _gen_ident(rng: Rng, n: int) -> str:
    alpha = "abcdefghijklmnopqrstuvwxyz"
    out = [alpha[rng.randint(0, len(alpha) - 1)] for _ in range(n)]
    return "".join(out)


def _gen_expr(rng: Rng, depth: int) -> str:
    if depth <= 0:
        # leaf
        k = rng.randint(0, 3)
        if k == 0:
            return str(rng.randint(-1000, 1000))
        if k == 1:
            return "true" if (rng.next_u32() & 1) else "false"
        if k == 2:
            return _gen_ident(rng, rng.randint(1, 8))
        return f"({_gen_expr(rng, 0)})"

    op = rng.choice(["+", "-", "*", "/", "<", ">", "==", "&&", "||"])
    a = _gen_expr(rng, depth - 1)
    b = _gen_expr(rng, depth - 1)
    return f"({a} {op} {b})"


def random_cases(seed: str, *, count: int, stress: int, logger: Logger) -> List[Case]:
    """Generate additional deterministic stress cases."""

    if count <= 0:
        return []

    rng = Rng.from_seed(seed)
    out: List[Case] = []

    # Stress scaling
    # 0: small
    # 1: medium
    # 2+: heavy
    expr_depth = 2 + stress
    stmt_count = 5 + stress * 10
    nesting = 2 + stress * 3

    for i in range(count):
        name = f"rand_{i:04d}"
        cid = _case_id(seed, name)

        # Build a small program with variables and nested ifs.
        vars_n = 3 + (rng.randint(0, 3) if stress > 0 else 2)
        var_names = [f"v{j}_{_gen_ident(rng, 4)}" for j in range(vars_n)]

        lines: List[str] = []
        lines.append("fn main()")

        for vn in var_names:
            lines.append(f"  let {vn} = {rng.randint(-50, 50)}")

        for s in range(stmt_count):
            target = rng.choice(var_names)
            expr = _gen_expr(rng, expr_depth)
            lines.append(f"  set {target} = {expr}")

        # nested control flow
        for d in range(nesting):
            cond = _gen_expr(rng, 1 + (stress // 2))
            lines.append(f"  if {cond}")

        lines.append("    ret 0")

        for _d in range(nesting):
            lines.append("  .end")

        lines.append(".end")

        input_vitte = "\n".join(lines) + "\n"

        exp = _contract_skeleton(
            case_id=cid,
            name=name,
            kind="stress",
            notes="Random stress fixture. Expected contract is skeleton; compare invariants only unless adapter is available.",
            invariants=[
                "lowering terminates",
                "no crash on deep nesting",
                "short-circuit semantics preserved if used",
            ],
        )

        out.append(
            Case(
                id=cid,
                name=name,
                description=f"Random stress case {i}",
                input_vitte=input_vitte,
                expected=exp,
                tags=("stress",),
            )
        )

    logger.debug("lower: random", "count", len(out), "stress", stress)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(case: Case) -> str:
    # <id>_<name> with safe chars
    name = case.name.replace(" ", "_").replace("/", "_")
    return f"{case.id}_{name}"


def emit(out_dir: Path, *, seed: str, count: int, stress: int, emit_md: bool, logger: Logger) -> None:
    curated = curated_cases(seed)
    rnd = random_cases(seed, count=count, stress=stress, logger=logger)

    cases = curated + rnd

    # Stable ordering: curated first (name order), then random (name order)
    cases.sort(key=lambda c: (c.tags != (), c.name, c.id))

    logger.info("lower: emit", "curated", len(curated), "random", len(rnd), "total", len(cases), "out", str(out_dir))

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        index_cases: List[Dict[str, Any]] = []

        for c in cases:
            d = _safe_dir_name(c)
            base = f"cases/{d}"

            w.write_text(f"{base}/input.vitte", c.input_vitte)
            w.write_text(f"{base}/expected.lower.json", dumps_json_stable(c.expected, indent=2) + "\n")

            meta = {
                "id": c.id,
                "name": c.name,
                "description": c.description,
                "tags": list(c.tags),
            }
            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")

            index_cases.append(
                {
                    "id": c.id,
                    "name": c.name,
                    "path": base,
                    "tags": list(c.tags),
                    "description": c.description,
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_lower.py",
            "seed": seed,
            "count": count,
            "stress": stress,
            "contract": {"name": "vitte.lower.contract", "version": 1},
            "counts": {
                "total": len(cases),
                "curated": len(curated),
                "random": len(rnd),
            },
            "cases": index_cases,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Lowering fixtures")
    lines.append("")
    lines.append("Fixtures for the Vitte lowering stage (AST -> IR-normalized form).")
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
    lines.append("- Keep directory names stable once published.")
    lines.append("- Expected outputs are a contract; adapt your internal IR in tests.")
    lines.append("")
    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Lowering fixtures index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- stress: `{index.get('stress',0)}`")
    lines.append("")
    lines.append("| id | name | path | tags | description |")
    lines.append("|---|---|---|---|---|")
    for c in index.get("cases", []):
        cid = str(c.get("id", ""))
        name = str(c.get("name", ""))
        path = str(c.get("path", ""))
        tags = ",".join([str(t) for t in c.get("tags", [])])
        desc = str(c.get("description", ""))
        lines.append(f"| `{cid}` | `{name}` | `{path}` | `{tags}` | {desc} |")
    lines.append("")
    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_lower.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--count", type=int, default=0, help="Number of random stress cases")
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
        _eprint(f"error: gen_lower failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())