

#!/usr/bin/env python3
"""gen_sema.py

Generate deterministic semantic-analysis fixtures for Vitte.

Scope
- The semantic layer ("sema") tends to regress on:
  - name resolution (shadowing, scopes, imports)
  - type checking (basic scalars, user types)
  - control-flow validity (break/continue/return positioning)
  - pattern/match exhaustiveness (basic)
  - diagnostics stability (codes, severity, spans)

This generator emits *two* classes of fixtures:
- ok/: sources that should pass semantic analysis
- err/: sources that should fail, with a stable diagnostics contract

Outputs (under --out):
  sema/
    index.json
    README.md
    INDEX.md                      (optional)
    ok/
      <id>_<name>/
        input.vitte
        expected.sema.json         # stable contract summary
        meta.json
    err/
      <id>_<name>/
        input.vitte
        expected.diag.json         # stable diagnostics contract
        meta.json

Contracts
- expected.sema.json
  - should_semantic_pass=true
  - stable summary counts (symbols, functions, types, warnings)
- expected.diag.json
  - should_semantic_pass=false
  - list of diagnostic entries (code, severity, message_stub, primary_span)

The contracts are intentionally minimal; an adapter can enrich them.

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_sema.py --out ../generated/sema --seed 0
  python3 gen_sema.py --out ../generated/sema --seed 0 --count 50 --stress 2 --emit-md
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
# Contracts
# -----------------------------------------------------------------------------


def sema_ok_contract(
    *,
    case_id: str,
    name: str,
    kind: str,
    description: str,
    counts: Dict[str, int],
    notes: str = "",
) -> Dict[str, Any]:
    return {
        "contract": {"name": "vitte.sema.ok", "version": 1},
        "case": {"id": case_id, "name": name, "kind": kind},
        "description": description,
        "should_semantic_pass": True,
        "counts": dict(counts),
        "notes": notes,
    }


def sema_err_contract(
    *,
    case_id: str,
    name: str,
    kind: str,
    description: str,
    diags: List[Dict[str, Any]],
    notes: str = "",
) -> Dict[str, Any]:
    return {
        "contract": {"name": "vitte.sema.diag", "version": 1},
        "case": {"id": case_id, "name": name, "kind": kind},
        "description": description,
        "should_semantic_pass": False,
        "diagnostics": diags,
        "notes": notes,
    }


# -----------------------------------------------------------------------------
# Case model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class OkCase:
    id: str
    name: str
    description: str
    kind: str
    input_vitte: str
    expected: Dict[str, Any]
    tags: Tuple[str, ...] = ()


@dataclass(frozen=True)
class ErrCase:
    id: str
    name: str
    description: str
    kind: str
    input_vitte: str
    expected: Dict[str, Any]
    tags: Tuple[str, ...] = ()


def _case_id(seed: str, name: str) -> str:
    h = hashlib.sha256()
    h.update(seed.encode("utf-8"))
    h.update(b"\n")
    h.update(name.encode("utf-8"))
    return h.hexdigest()[:16]


def _mk_ok(
    *,
    seed: str,
    name: str,
    description: str,
    kind: str,
    input_vitte: str,
    counts: Dict[str, int],
    notes: str,
    tags: Sequence[str] = (),
) -> OkCase:
    cid = _case_id(seed, name)
    exp = sema_ok_contract(case_id=cid, name=name, kind=kind, description=description, counts=counts, notes=notes)
    return OkCase(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        input_vitte=input_vitte,
        expected=exp,
        tags=tuple(tags),
    )


def _mk_err(
    *,
    seed: str,
    name: str,
    description: str,
    kind: str,
    input_vitte: str,
    diags: List[Dict[str, Any]],
    notes: str,
    tags: Sequence[str] = (),
) -> ErrCase:
    cid = _case_id(seed, name)
    exp = sema_err_contract(case_id=cid, name=name, kind=kind, description=description, diags=diags, notes=notes)
    return ErrCase(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        input_vitte=input_vitte,
        expected=exp,
        tags=tuple(tags),
    )


# -----------------------------------------------------------------------------
# Curated OK cases
# -----------------------------------------------------------------------------


def curated_ok(seed: str) -> List[OkCase]:
    ok: List[OkCase] = []

    ok.append(
        _mk_ok(
            seed=seed,
            name="ok_shadowing_scopes",
            description="Shadowing across nested blocks",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let x = 1\n"
                "  if true\n"
                "    let x = 2\n"
                "    set x = x + 1\n"
                "  .end\n"
                "  ret x\n"
                ".end\n"
            ),
            counts={"functions": 1, "locals": 2, "warnings": 0, "errors": 0},
            notes="Sema should handle shadowing with distinct bindings.",
            tags=["scope", "shadow"],
        )
    )

    ok.append(
        _mk_ok(
            seed=seed,
            name="ok_user_types",
            description="User types + function signatures",
            kind="core",
            input_vitte=(
                "struct Point\n"
                "  x: i32\n"
                "  y: i32\n"
                ".end\n"
                "fn make(x: i32, y: i32) -> Point\n"
                "  let p: Point\n"
                "  ret p\n"
                ".end\n"
            ),
            counts={"functions": 1, "types": 1, "warnings": 0, "errors": 0},
            notes="Type presence + name resolution for Point.",
            tags=["types"],
        )
    )

    ok.append(
        _mk_ok(
            seed=seed,
            name="ok_imports",
            description="Imports and symbol references",
            kind="core",
            input_vitte=(
                "module pkg/main\n"
                "use pkg/lib\n"
                "fn main()\n"
                "  let x = add(1, 2)\n"
                "  ret x\n"
                ".end\n"
            ),
            counts={"functions": 1, "imports": 1, "warnings": 0, "errors": 0},
            notes="Assumes resolver can map imported symbols in test harness.",
            tags=["imports"],
        )
    )

    ok.append(
        _mk_ok(
            seed=seed,
            name="ok_control_flow",
            description="Valid break/continue/return placement",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let s = 0\n"
                "  while s < 10\n"
                "    set s = s + 1\n"
                "    if s == 5\n"
                "      continue\n"
                "    .end\n"
                "    if s == 9\n"
                "      break\n"
                "    .end\n"
                "  .end\n"
                "  ret s\n"
                ".end\n"
            ),
            counts={"functions": 1, "loops": 1, "warnings": 0, "errors": 0},
            notes="Break/continue legal inside loops.",
            tags=["flow"],
        )
    )

    return ok


# -----------------------------------------------------------------------------
# Curated ERR cases (diagnostics)
# -----------------------------------------------------------------------------


def curated_err(seed: str) -> List[ErrCase]:
    err: List[ErrCase] = []

    err.append(
        _mk_err(
            seed=seed,
            name="err_undefined_name",
            description="Undefined identifier reference",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  set x = y + 1\n"
                "  ret 0\n"
                ".end\n"
            ),
            diags=[
                {
                    "code": "E_UNDEFINED_NAME",
                    "severity": "error",
                    "message_stub": "undefined name",
                    "primary_span": {"line": 2, "col": 11, "len": 1},
                }
            ],
            notes="`y` is not declared.",
            tags=["names"],
        )
    )

    err.append(
        _mk_err(
            seed=seed,
            name="err_duplicate_decl",
            description="Duplicate declaration in same scope",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let x = 1\n"
                "  let x = 2\n"
                "  ret x\n"
                ".end\n"
            ),
            diags=[
                {
                    "code": "E_DUPLICATE_DECL",
                    "severity": "error",
                    "message_stub": "duplicate",
                    "primary_span": {"line": 3, "col": 7, "len": 1},
                }
            ],
            notes="Second `x` in same scope should error.",
            tags=["names"],
        )
    )

    err.append(
        _mk_err(
            seed=seed,
            name="err_break_outside_loop",
            description="break outside loop",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  break\n"
                "  ret 0\n"
                ".end\n"
            ),
            diags=[
                {
                    "code": "E_BREAK_OUTSIDE_LOOP",
                    "severity": "error",
                    "message_stub": "break outside loop",
                    "primary_span": {"line": 2, "col": 3, "len": 5},
                }
            ],
            notes="Control-flow validation.",
            tags=["flow"],
        )
    )

    err.append(
        _mk_err(
            seed=seed,
            name="err_type_mismatch",
            description="Type mismatch in assignment",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let x: i32\n"
                "  set x = \"str\"\n"
                "  ret 0\n"
                ".end\n"
            ),
            diags=[
                {
                    "code": "E_TYPE_MISMATCH",
                    "severity": "error",
                    "message_stub": "type mismatch",
                    "primary_span": {"line": 3, "col": 11, "len": 5},
                }
            ],
            notes="Assigning string to i32.",
            tags=["types"],
        )
    )

    return err


# -----------------------------------------------------------------------------
# Random OK/ERR generation
# -----------------------------------------------------------------------------


def _gen_ident(rng: Rng, n: int) -> str:
    alpha = "abcdefghijklmnopqrstuvwxyz"
    return "".join(alpha[rng.randint(0, len(alpha) - 1)] for _ in range(n))


def random_ok(seed: str, *, count: int, stress: int, logger: Logger) -> List[OkCase]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":sema:ok")
    out: List[OkCase] = []

    for i in range(count):
        name = f"rand_ok_{i:04d}"

        vars_n = 3 + stress
        vars_ = [f"v{j}_{_gen_ident(rng, 4)}" for j in range(vars_n)]

        lines: List[str] = ["fn main()"]
        for v in vars_:
            lines.append(f"  let {v} = {rng.randint(0, 10)}")

        # Some legal arithmetic and loops
        loops = 1 + (stress // 2)
        for _ in range(loops):
            lines.append(f"  while {vars_[0]} < {10 + stress * 10}")
            for _k in range(2 + stress * 2):
                t = rng.choice(vars_)
                u = rng.choice(vars_)
                lines.append(f"    set {t} = {t} + {u}")
            lines.append("  .end")

        lines.append(f"  ret {vars_[0]}")
        lines.append(".end")

        src = "\n".join(lines) + "\n"

        out.append(
            _mk_ok(
                seed=seed,
                name=name,
                description=f"Random OK sema case {i}",
                kind="core",
                input_vitte=src,
                counts={"functions": 1, "locals": vars_n, "warnings": 0, "errors": 0},
                notes="Generated valid; adapter may compute richer stats.",
                tags=["random", "ok"],
            )
        )

    logger.debug("sema: random ok", "count", len(out), "stress", stress)
    return out


def random_err(seed: str, *, count: int, stress: int, logger: Logger) -> List[ErrCase]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":sema:err")
    out: List[ErrCase] = []

    for i in range(count):
        name = f"rand_err_{i:04d}"

        # Mix: undefined name or break outside loop
        if rng.random() < 0.5:
            # undefined
            v = _gen_ident(rng, 5)
            src = (
                "fn main()\n"
                f"  set {v} = missing + 1\n"
                "  ret 0\n"
                ".end\n"
            )
            diags = [
                {
                    "code": "E_UNDEFINED_NAME",
                    "severity": "error",
                    "message_stub": "undefined name",
                    "primary_span": {"line": 2, "col": 13, "len": 7},
                }
            ]
            desc = f"Random ERR undefined name {i}"
            tags = ["random", "err", "names"]
        else:
            # break outside loop
            extra = "\n".join(["  let x = 0" for _ in range(stress)])
            src = "fn main()\n" + (extra + "\n" if extra else "") + "  break\n  ret 0\n.end\n"
            diags = [
                {
                    "code": "E_BREAK_OUTSIDE_LOOP",
                    "severity": "error",
                    "message_stub": "break outside loop",
                    "primary_span": {"line": 2 + (stress if stress > 0 else 0), "col": 3, "len": 5},
                }
            ]
            desc = f"Random ERR break outside loop {i}"
            tags = ["random", "err", "flow"]

        out.append(
            _mk_err(
                seed=seed,
                name=name,
                description=desc,
                kind="core",
                input_vitte=src,
                diags=diags,
                notes="Generated invalid; codes are contract-level and may map in adapter.",
                tags=tags,
            )
        )

    logger.debug("sema: random err", "count", len(out), "stress", stress)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(cid: str, name: str) -> str:
    n = name.replace(" ", "_").replace("/", "_")
    return f"{cid}_{n}"


def emit(out_dir: Path, *, seed: str, count: int, stress: int, emit_md: bool, logger: Logger) -> None:
    ok_cur = curated_ok(seed)
    err_cur = curated_err(seed)

    ok_rnd = random_ok(seed, count=count, stress=stress, logger=logger)
    err_rnd = random_err(seed, count=count, stress=stress, logger=logger)

    ok = ok_cur + ok_rnd
    err = err_cur + err_rnd

    ok.sort(key=lambda c: ("random" in c.tags, c.name, c.id))
    err.sort(key=lambda c: ("random" in c.tags, c.name, c.id))

    logger.info(
        "sema: emit",
        "ok",
        len(ok),
        "err",
        len(err),
        "out",
        str(out_dir),
    )

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        ok_index: List[Dict[str, Any]] = []
        err_index: List[Dict[str, Any]] = []

        for c in ok:
            d = _safe_dir_name(c.id, c.name)
            base = f"ok/{d}"
            w.write_text(f"{base}/input.vitte", c.input_vitte)
            w.write_text(f"{base}/expected.sema.json", dumps_json_stable(c.expected, indent=2) + "\n")
            meta = {
                "id": c.id,
                "name": c.name,
                "kind": c.kind,
                "description": c.description,
                "tags": list(c.tags),
            }
            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")
            ok_index.append(
                {
                    "id": c.id,
                    "name": c.name,
                    "kind": c.kind,
                    "path": base,
                    "tags": list(c.tags),
                    "description": c.description,
                }
            )

        for c in err:
            d = _safe_dir_name(c.id, c.name)
            base = f"err/{d}"
            w.write_text(f"{base}/input.vitte", c.input_vitte)
            w.write_text(f"{base}/expected.diag.json", dumps_json_stable(c.expected, indent=2) + "\n")
            meta = {
                "id": c.id,
                "name": c.name,
                "kind": c.kind,
                "description": c.description,
                "tags": list(c.tags),
            }
            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")
            err_index.append(
                {
                    "id": c.id,
                    "name": c.name,
                    "kind": c.kind,
                    "path": base,
                    "tags": list(c.tags),
                    "description": c.description,
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_sema.py",
            "seed": seed,
            "count": count,
            "stress": stress,
            "contracts": {
                "ok": {"name": "vitte.sema.ok", "version": 1},
                "diag": {"name": "vitte.sema.diag", "version": 1},
            },
            "counts": {
                "ok_total": len(ok),
                "ok_curated": len(ok_cur),
                "ok_random": len(ok_rnd),
                "err_total": len(err),
                "err_curated": len(err_cur),
                "err_random": len(err_rnd),
            },
            "ok": ok_index,
            "err": err_index,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Sema fixtures")
    lines.append("")
    lines.append("Semantic-analysis fixtures for Vitte.")
    lines.append("")

    counts = index.get("counts", {})
    lines.append("## Counts")
    lines.append("")
    for k, v in sorted(counts.items()):
        lines.append(f"- {k}: **{v}**")

    lines.append("")
    lines.append("## Contracts")
    lines.append("")
    cs = index.get("contracts", {})
    okc = cs.get("ok", {})
    dc = cs.get("diag", {})
    lines.append(f"- ok: `{okc.get('name','')}` v{okc.get('version',0)}")
    lines.append(f"- diag: `{dc.get('name','')}` v{dc.get('version',0)}")

    lines.append("")
    lines.append("## Policy")
    lines.append("")
    lines.append("- Prefer adding new cases instead of modifying existing ones.")
    lines.append("- Diagnostics are a contract; map internal compiler diags to these codes in adapters.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Sema fixtures index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- stress: `{index.get('stress',0)}`")
    lines.append("")

    lines.append("## OK cases")
    lines.append("")
    lines.append("| id | name | kind | path | tags | description |")
    lines.append("|---|---|---|---|---|---|")
    for c in index.get("ok", []):
        lines.append(
            "| `{}` | `{}` | `{}` | `{}` | `{}` | {} |".format(
                c.get("id", ""),
                c.get("name", ""),
                c.get("kind", ""),
                c.get("path", ""),
                ",".join([str(t) for t in c.get("tags", [])]),
                c.get("description", ""),
            )
        )

    lines.append("")
    lines.append("## ERR cases")
    lines.append("")
    lines.append("| id | name | kind | path | tags | description |")
    lines.append("|---|---|---|---|---|---|")
    for c in index.get("err", []):
        lines.append(
            "| `{}` | `{}` | `{}` | `{}` | `{}` | {} |".format(
                c.get("id", ""),
                c.get("name", ""),
                c.get("kind", ""),
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
    ap = argparse.ArgumentParser(prog="gen_sema.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--count", type=int, default=0, help="Random cases per kind")
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
        _eprint(f"error: gen_sema failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())