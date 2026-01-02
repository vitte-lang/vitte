

#!/usr/bin/env python3
"""gen_numbers.py

Generate numeric-literal corpora for Vitte benches and fuzzing.

Why
- Numeric literal parsing is a high-value fuzz surface (bases, underscores,
  exponents, overflows, edge ranges).
- Benches often need deterministic numeric datasets.

Outputs (under --out):
  numbers/
    index.json
    README.md
    INDEX.md                      (optional)
    cases/
      <id>_<name>/
        literals.txt              # one literal per line
        literals.json             # stable JSON list of literals
        vitte_literals.vitte      # Vitte source embedding literals
        meta.json

Notes
- This generator focuses on *valid* literals for the language surface.
- Overflow semantics are not assumed; we include boundary-adjacent values.
- Add new cases over time; avoid modifying existing file names.

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_numbers.py --out ../generated/numbers --seed 0
  python3 gen_numbers.py --out ../generated/numbers --seed 1 --count 50 --size 200 --emit-md
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
# Case model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class Case:
    id: str
    name: str
    description: str
    kind: str
    literals: Tuple[str, ...]
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
    literals: Sequence[str],
    tags: Sequence[str] = (),
) -> Case:
    cid = _case_id(seed, name)
    return Case(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        literals=tuple(literals),
        tags=tuple(tags),
    )


# -----------------------------------------------------------------------------
# Literal builders
# -----------------------------------------------------------------------------


def _fmt_int_dec(n: int) -> str:
    return str(n)


def _fmt_int_hex(n: int) -> str:
    if n < 0:
        return "-0x" + format(-n, "x")
    return "0x" + format(n, "x")


def _fmt_int_bin(n: int) -> str:
    if n < 0:
        return "-0b" + format(-n, "b")
    return "0b" + format(n, "b")


def _with_underscores(digits: str, rng: Rng, *, prob: float = 0.35, min_group: int = 3, max_group: int = 4) -> str:
    """Insert underscores in a digit string deterministically."""

    if len(digits) < (min_group + 1):
        return digits

    # decide if we underscore
    if rng.random() > prob:
        return digits

    out: List[str] = []
    i = 0
    while i < len(digits):
        g = rng.randint(min_group, max_group)
        out.append(digits[i : i + g])
        i += g
        if i < len(digits):
            out.append("_")
    s = "".join(out)
    # avoid trailing underscore
    if s.endswith("_"):
        s = s[:-1]
    return s


def _fmt_float(rng: Rng, mant: float, exp: int) -> str:
    # Use decimal scientific notation; keep stable formatting.
    # Example: 1.2345e-10
    # Clamp mantissa to a sane printable range.
    if mant == 0.0:
        return "0.0"

    sign = "-" if mant < 0 else ""
    mant = abs(mant)

    # Keep 1..9.xxx
    while mant >= 10.0:
        mant /= 10.0
        exp += 1
    while mant < 1.0:
        mant *= 10.0
        exp -= 1

    # fixed 6 decimal digits for stability
    s = f"{mant:.6f}"

    # optionally underscore integer part (rare)
    if rng.random() < 0.10:
        if "." in s:
            a, b = s.split(".", 1)
            a = _with_underscores(a, rng, prob=1.0, min_group=1, max_group=2)
            s = a + "." + b

    if exp == 0:
        return sign + s
    return sign + s + ("e" + str(exp))


# -----------------------------------------------------------------------------
# Curated cases
# -----------------------------------------------------------------------------


def curated_cases(seed: str) -> List[Case]:
    rng = Rng.from_seed(seed + ":curated")

    # Boundary-adjacent values for common widths.
    i8 = [-128, -127, -1, 0, 1, 126, 127]
    u8 = [0, 1, 254, 255]
    i16 = [-32768, -32767, -1, 0, 1, 32766, 32767]
    u16 = [0, 1, 65534, 65535]
    i32 = [-2147483648, -2147483647, -1, 0, 1, 2147483646, 2147483647]
    u32 = [0, 1, 4294967294, 4294967295]
    i64 = [-9223372036854775808, -9223372036854775807, -1, 0, 1, 9223372036854775806, 9223372036854775807]
    u64 = [0, 1, 18446744073709551614, 18446744073709551615]

    ints: List[str] = []
    for v in i8 + u8 + i16 + u16 + i32 + u32 + i64 + u64:
        ints.append(_fmt_int_dec(v))

    # Mixed bases.
    bases: List[str] = []
    for v in [0, 1, 2, 7, 8, 15, 16, 31, 32, 63, 64, 127, 255, 1024, 65535, 1048576]:
        bases.append(_fmt_int_hex(v))
        bases.append(_fmt_int_bin(v))
    for v in [-1, -2, -15, -16, -1024]:
        bases.append(_fmt_int_hex(v))
        bases.append(_fmt_int_bin(v))

    # Underscore forms (decimal + hex + bin)
    under: List[str] = []
    for v in [
        1_000,
        10_000,
        1_000_000,
        123_456_789,
        0xDEADBEEF,
        0x12345678,
        0b1010101010,
        0b111100001111,
    ]:
        # represent as digits then insert underscores
        if isinstance(v, int):
            # For bases, preserve prefix then underscore the payload.
            s = str(v)
            if s.startswith("0x") or s.startswith("0b"):
                under.append(s)
            else:
                digits = s.lstrip("-")
                us = _with_underscores(digits, rng, prob=1.0)
                under.append(("-" if s.startswith("-") else "") + us)

    # Floats: typical decimal and exponent forms.
    floats: List[str] = [
        "0.0",
        "1.0",
        "-1.0",
        "3.141592",
        "-0.125",
        "1e0",
        "1e-9",
        "1e9",
        "1.5e2",
        "-2.5e-3",
        "0.000001",
    ]

    # Add a few deterministic generated scientific floats.
    for _ in range(32):
        mant = (rng.randint(-999999, 999999) / 100000.0) or 1.0
        exp = rng.randint(-30, 30)
        floats.append(_fmt_float(rng, mant, exp))

    # Keep stable order.
    def uniq_keep(xs: List[str]) -> List[str]:
        seen = set()
        out: List[str] = []
        for x in xs:
            if x in seen:
                continue
            seen.add(x)
            out.append(x)
        return out

    ints = uniq_keep(ints)
    bases = uniq_keep(bases)
    under = uniq_keep(under)
    floats = uniq_keep(floats)

    return [
        _mk_case(
            seed=seed,
            name="int_boundaries",
            description="Boundary-adjacent decimal integers (common widths)",
            kind="int",
            literals=ints,
            tags=["int", "boundary"],
        ),
        _mk_case(
            seed=seed,
            name="int_bases",
            description="Hex and binary integer literals (including negatives)",
            kind="int",
            literals=bases,
            tags=["int", "base"],
        ),
        _mk_case(
            seed=seed,
            name="int_underscores",
            description="Integer literals with underscore grouping",
            kind="int",
            literals=under,
            tags=["int", "underscore"],
        ),
        _mk_case(
            seed=seed,
            name="float_mixed",
            description="Float literals in decimal and scientific notation",
            kind="float",
            literals=floats,
            tags=["float"],
        ),
    ]


# -----------------------------------------------------------------------------
# Random cases
# -----------------------------------------------------------------------------


def _rand_digits(rng: Rng, n: int) -> str:
    return "".join(str(rng.randint(0, 9)) for _ in range(n))


def _rand_int_literal(rng: Rng) -> str:
    sign = "-" if (rng.next_u32() & 1) else ""
    base_kind = rng.randint(0, 2)

    if base_kind == 0:
        # decimal
        digits = _rand_digits(rng, rng.randint(1, 40))
        digits = digits.lstrip("0") or "0"
        digits = _with_underscores(digits, rng, prob=0.45)
        return sign + digits

    if base_kind == 1:
        # hex
        digits = "".join(rng.choice("0123456789abcdef") for _ in range(rng.randint(1, 32)))
        digits = digits.lstrip("0") or "0"
        # underscores in payload
        if rng.random() < 0.35:
            digits = _with_underscores(digits, rng, prob=1.0, min_group=2, max_group=4)
        return sign + "0x" + digits

    # bin
    digits = "".join("1" if (rng.next_u32() & 1) else "0" for _ in range(rng.randint(1, 128)))
    digits = digits.lstrip("0") or "0"
    if rng.random() < 0.35:
        digits = _with_underscores(digits, rng, prob=1.0, min_group=4, max_group=8)
    return sign + "0b" + digits


def _rand_float_literal(rng: Rng) -> str:
    sign = "-" if (rng.next_u32() & 1) else ""

    # base mantissa
    int_part = _rand_digits(rng, rng.randint(1, 10)).lstrip("0") or "0"
    frac_part = _rand_digits(rng, rng.randint(0, 12))

    if rng.random() < 0.35 and len(int_part) > 3:
        int_part = _with_underscores(int_part, rng, prob=1.0)

    if frac_part:
        s = int_part + "." + frac_part
    else:
        # force a float token
        s = int_part + ".0"

    if rng.random() < 0.55:
        exp = rng.randint(-300, 300)
        s = s + "e" + str(exp)

    return sign + s


def random_cases(seed: str, *, count: int, size: int, logger: Logger) -> List[Case]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":random")
    out: List[Case] = []

    for i in range(count):
        name = f"rand_{i:04d}"
        cid = _case_id(seed, name)
        kind = "mix" if (i % 3 == 0) else ("int" if (i % 3 == 1) else "float")

        lits: List[str] = []
        for _ in range(size):
            if kind == "int":
                lits.append(_rand_int_literal(rng))
            elif kind == "float":
                lits.append(_rand_float_literal(rng))
            else:
                # mix
                if rng.random() < 0.6:
                    lits.append(_rand_int_literal(rng))
                else:
                    lits.append(_rand_float_literal(rng))

        # Keep stable order; drop duplicates to reduce file size a bit.
        seen = set()
        uniq: List[str] = []
        for x in lits:
            if x in seen:
                continue
            seen.add(x)
            uniq.append(x)

        out.append(
            Case(
                id=cid,
                name=name,
                description=f"Random numeric literals ({kind}), size={len(uniq)}",
                kind=kind,
                literals=tuple(uniq),
                tags=("random", kind),
            )
        )

    logger.debug("numbers: random", "count", len(out), "size", size)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(case: Case) -> str:
    name = case.name.replace(" ", "_").replace("/", "_")
    return f"{case.id}_{name}"


def _emit_vitte_source(case: Case) -> str:
    """Embed literals into a Vitte program.

    This isn't meant to compile for all targets; it's a parsing/formatter surface.
    """

    lines: List[str] = []
    lines.append("fn main()")

    # Chunk to avoid extremely long lines.
    chunk = 16
    lits = list(case.literals)

    # Create variables a0..an.
    idx = 0
    for i in range(0, len(lits), chunk):
        part = lits[i : i + chunk]
        for lit in part:
            lines.append(f"  let a{idx} = {lit}")
            idx += 1

    lines.append("  ret 0")
    lines.append(".end")
    return "\n".join(lines) + "\n"


def emit(out_dir: Path, *, seed: str, count: int, size: int, emit_md: bool, logger: Logger) -> None:
    curated = curated_cases(seed)
    rnd = random_cases(seed, count=count, size=size, logger=logger)

    cases = curated + rnd

    # Stable ordering: curated first, then random by name.
    cases.sort(key=lambda c: ("random" in c.tags, c.name, c.id))

    logger.info("numbers: emit", "curated", len(curated), "random", len(rnd), "total", len(cases), "out", str(out_dir))

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        index_cases: List[Dict[str, Any]] = []

        for c in cases:
            d = _safe_dir_name(c)
            base = f"cases/{d}"

            # literals.txt
            w.write_text(f"{base}/literals.txt", "\n".join(c.literals) + "\n")

            # literals.json
            w.write_text(
                f"{base}/literals.json",
                dumps_json_stable({"kind": c.kind, "literals": list(c.literals)}, indent=2) + "\n",
            )

            # vitte_literals.vitte
            w.write_text(f"{base}/vitte_literals.vitte", _emit_vitte_source(c))

            meta = {
                "id": c.id,
                "name": c.name,
                "description": c.description,
                "kind": c.kind,
                "tags": list(c.tags),
                "counts": {"literals": len(c.literals)},
            }
            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")

            index_cases.append(
                {
                    "id": c.id,
                    "name": c.name,
                    "path": base,
                    "kind": c.kind,
                    "tags": list(c.tags),
                    "description": c.description,
                    "counts": {"literals": len(c.literals)},
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_numbers.py",
            "seed": seed,
            "count": count,
            "size": size,
            "contract": {"name": "vitte.numbers.v1", "version": 1},
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
    lines.append("# Numbers corpus")
    lines.append("")
    lines.append("Deterministic corpora for numeric-literal parsing and benches.")
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
    lines.append("- Keep literals valid for the language surface.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Numbers corpus index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- size: `{index.get('size',0)}`")
    lines.append("")
    lines.append("| id | name | kind | path | tags | literals | description |")
    lines.append("|---|---|---|---|---|---:|---|")
    for c in index.get("cases", []):
        cid = str(c.get("id", ""))
        name = str(c.get("name", ""))
        kind = str(c.get("kind", ""))
        path = str(c.get("path", ""))
        tags = ",".join([str(t) for t in c.get("tags", [])])
        nlit = int(c.get("counts", {}).get("literals", 0))
        desc = str(c.get("description", ""))
        lines.append(f"| `{cid}` | `{name}` | `{kind}` | `{path}` | `{tags}` | {nlit} | {desc} |")
    lines.append("")
    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_numbers.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--count", type=int, default=0, help="Number of random cases")
    ap.add_argument("--size", type=int, default=200, help="Literals per random case")
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
    if ns.size <= 0:
        _eprint("error: --size must be > 0")
        return 2

    log = Logger(level="debug" if ns.verbose else "info", quiet=bool(ns.quiet))

    try:
        emit(out_dir, seed=str(ns.seed), count=int(ns.count), size=int(ns.size), emit_md=bool(ns.emit_md), logger=log)
    except Exception as e:
        _eprint(f"error: gen_numbers failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())