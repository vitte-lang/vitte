

#!/usr/bin/env python3
"""gen_unicode_idents.py

Generate deterministic *unicode identifier* fixtures for Vitte.

Rationale
Unicode identifiers are a recurrent source of regressions across:
- lexer (classification, boundary detection)
- parser (identifier tokens in paths, fields, etc.)
- name resolution (distinct bindings, shadowing)
- formatting/printing (stable round-trips)
- tooling (LSP, diagnostics spans)

This generator is implementation-agnostic: it emits inputs and a compact
"reference analysis" computed via Python's Unicode tables.

Outputs (under --out):
  unicode_idents/
    index.json
    README.md
    INDEX.md                        (optional)
    sets/
      <id>_<name>/
        input.vitte
        expected.unicode.json       # reference identifier extraction + properties
        meta.json

Contract
- expected.unicode.json
  - contract: vitte.unicode_idents.v1
  - identifiers: list of extracted identifiers with:
      - text
      - codepoints (hex)
      - nfc/nfd
      - category_summary (counts by unicode category prefix: L, M, N, P, S, Z, C)

Notes
- This script does *not* decide whether a given identifier is "valid" per the
  compiler rules. It only offers a stable baseline signal for comparison.

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_unicode_idents.py --out ../generated/unicode_idents --seed 0
  python3 gen_unicode_idents.py --out ../generated/unicode_idents --seed 0 --count 50 --stress 2 --emit-md
"""

from __future__ import annotations

import argparse
import hashlib
import re
import unicodedata
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple

from vitte_benchgen.rng import Rng
from vitte_benchgen.util import Logger, dumps_json_stable
from vitte_benchgen.writer import StagingWriter, WriteOptions


# -----------------------------------------------------------------------------
# Reference identifier extraction
# -----------------------------------------------------------------------------


# Conservative unicode ident-ish matcher:
# - Start: any unicode word char that is not a digit
# - Continue: unicode word chars
# This matches more than many language specs, but is stable and useful.
_RE_IDENT = re.compile(r"(?!\d)\w+", re.UNICODE)


def _codepoints(s: str) -> List[str]:
    return [f"U+{ord(ch):04X}" for ch in s]


def _category_bucket(ch: str) -> str:
    # Unicode general category: e.g. 'Ll', 'Mn', 'Nd', 'Zs', ...
    # We bucket by the first letter for compactness.
    cat = unicodedata.category(ch)
    return cat[:1] if cat else "?"


def _category_summary(s: str) -> Dict[str, int]:
    out: Dict[str, int] = {}
    for ch in s:
        b = _category_bucket(ch)
        out[b] = out.get(b, 0) + 1
    return out


def extract_identifiers(src: str) -> List[str]:
    # Filter out keywords-like tokens that start with '.' (e.g. '.end')
    # Those won't match \w anyway, but keep this hook stable.
    return _RE_IDENT.findall(src)


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
    idents: Tuple[str, ...]
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
    ids = tuple(extract_identifiers(input_vitte))
    return Case(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        input_vitte=input_vitte,
        idents=ids,
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
            name="latin_accents",
            description="Latin identifiers with accents",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let café = 1\n"
                "  let élève = café + 1\n"
                "  let façade = élève\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["latin", "accents"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="greek",
            description="Greek identifiers",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let μ = 1\n"
                "  let ΑΒΓ = μ + 1\n"
                "  let Δ = ΑΒΓ\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["greek"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="cyrillic",
            description="Cyrillic identifiers",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let Привет = 1\n"
                "  let мир = Привет + 1\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["cyrillic"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="cjk",
            description="CJK identifiers",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let 漢字 = 1\n"
                "  let 東京 = 漢字 + 1\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["cjk"],
        )
    )

    # Normalization pitfalls: NFC vs NFD forms that are visually identical.
    e_nfd = "e\u0301"  # 'e' + combining acute
    cases.append(
        _mk_case(
            seed=seed,
            name="combining_marks_nfd",
            description="Identifiers with combining marks (NFD sequence)",
            kind="core",
            input_vitte=(
                "fn main()\n"
                f"  let {e_nfd} = 1\n"
                f"  let {e_nfd}x = {e_nfd} + 1\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["combining", "nfd"],
        )
    )

    # Mixed scripts & confusable shapes: Latin 'C' vs Cyrillic 'С' (U+0421)
    lat_C = "C"
    cyr_C = "\u0421"  # Cyrillic capital ES
    cases.append(
        _mk_case(
            seed=seed,
            name="confusables_basic",
            description="Visually confusable identifiers across scripts",
            kind="core",
            input_vitte=(
                "fn main()\n"
                f"  let {lat_C} = 1\n"
                f"  let {cyr_C} = 2\n"
                f"  ret {lat_C}\n"
                ".end\n"
            ),
            tags=["confusables"],
        )
    )

    cases.append(
        _mk_case(
            seed=seed,
            name="unicode_in_comments_and_strings",
            description="Unicode in comments and strings alongside identifiers",
            kind="core",
            input_vitte=(
                "# café μ 漢字\n"
                "fn main()\n"
                "  let café = \"café\"\n"
                "  let μ = 1\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["comments", "strings"],
        )
    )

    return cases


# -----------------------------------------------------------------------------
# Random cases
# -----------------------------------------------------------------------------


def _rand_from_pool(rng: Rng, pool: Sequence[str]) -> str:
    return pool[rng.randint(0, len(pool) - 1)]


def _rand_unicode_ident(rng: Rng, *, stress: int) -> str:
    # Keep pools explicit and stable. Avoid control/whitespace.
    latin = [
        "café",
        "élève",
        "façade",
        "naïve",
        "über",
        "piñata",
        "SãoPaulo",
        "Łódź",
        "Żółć",
    ]
    greek = ["μ", "Δ", "λ", "Ω", "αβγ", "π", "σ"]
    cyr = ["Привет", "мир", "данные", "функция", "переменная"]
    cjk = ["漢字", "東京", "数据", "函数", "变量"]

    # Combining mark variants
    comb = [
        "e\u0301",  # e + acute
        "a\u0308",  # a + diaeresis
        "o\u0302",  # o + circumflex
    ]

    pools = [latin, greek, cyr, cjk, comb]

    base = _rand_from_pool(rng, _rand_from_pool(rng, pools))

    # Extend with ascii to create more ids and boundaries.
    suffix_len = 1 + rng.randint(0, 2 + stress)
    ascii = "abcdefghijklmnopqrstuvwxyz0123456789_"
    suffix = "".join(ascii[rng.randint(0, len(ascii) - 1)] for _ in range(suffix_len))

    # Ensure identifier doesn't start with digit (base already safe).
    return base + suffix


def random_cases(seed: str, *, count: int, stress: int, logger: Logger) -> List[Case]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":unicode_idents:random")
    out: List[Case] = []

    for i in range(count):
        name = f"rand_{i:04d}"

        n_vars = 6 + stress * 10
        ids = [_rand_unicode_ident(rng, stress=stress) for _ in range(n_vars)]

        lines: List[str] = []
        if rng.random() < 0.25:
            lines.append("# header café μ 漢字")

        lines.append("fn main()")

        # declare
        for j, v in enumerate(ids):
            if rng.random() < 0.20:
                lines.append(f"  let {v} = \"{v}\"")
            else:
                lines.append(f"  let {v} = {j}")

        # some references
        iters = 8 + stress * 30
        for _ in range(iters):
            a = ids[rng.randint(0, len(ids) - 1)]
            b = ids[rng.randint(0, len(ids) - 1)]
            t = ids[rng.randint(0, len(ids) - 1)]
            if rng.random() < 0.12:
                lines.append(f"  # mix {a} {b} {t}")
            lines.append(f"  set {t} = {a} + {b}")

        lines.append("  ret 0")
        lines.append(".end")

        src = "\n".join(lines) + "\n"

        out.append(
            _mk_case(
                seed=seed,
                name=name,
                description=f"Random unicode identifier fixture {i} (stress={stress})",
                kind="core",
                input_vitte=src,
                tags=["random", "stress" if stress else "random"],
            )
        )

    logger.debug("unicode_idents: random", "count", len(out), "stress", stress)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(c: Case) -> str:
    name = c.name.replace(" ", "_").replace("/", "_")
    return f"{c.id}_{name}"


def _case_contract(c: Case) -> Dict[str, Any]:
    uniq: List[str] = []
    seen: set[str] = set()
    for s in c.idents:
        if s not in seen:
            uniq.append(s)
            seen.add(s)

    idents_json: List[Dict[str, Any]] = []
    for s in uniq:
        idents_json.append(
            {
                "text": s,
                "codepoints": _codepoints(s),
                "nfc": unicodedata.normalize("NFC", s),
                "nfd": unicodedata.normalize("NFD", s),
                "category_summary": _category_summary(s),
            }
        )

    return {
        "contract": {"name": "vitte.unicode_idents.v1", "version": 1},
        "case": {"id": c.id, "name": c.name, "kind": c.kind},
        "description": c.description,
        "counts": {
            "ident_occurrences": len(c.idents),
            "ident_unique": len(uniq),
        },
        "identifiers": idents_json,
    }


def emit(out_dir: Path, *, seed: str, count: int, stress: int, emit_md: bool, logger: Logger) -> None:
    curated = curated_cases(seed)
    rnd = random_cases(seed, count=count, stress=stress, logger=logger)

    cases = curated + rnd
    cases.sort(key=lambda c: ("random" in c.tags, c.name, c.id))

    logger.info(
        "unicode_idents: emit",
        "curated",
        len(curated),
        "random",
        len(rnd),
        "total",
        len(cases),
        "out",
        str(out_dir),
    )

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        idx: List[Dict[str, Any]] = []

        for c in cases:
            d = _safe_dir_name(c)
            base = f"sets/{d}"

            w.write_text(f"{base}/input.vitte", c.input_vitte)
            w.write_text(f"{base}/expected.unicode.json", dumps_json_stable(_case_contract(c), indent=2) + "\n")

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
            "tool": "gen_unicode_idents.py",
            "seed": seed,
            "count": count,
            "stress": stress,
            "contract": {"name": "vitte.unicode_idents.v1", "version": 1},
            "counts": {"total": len(cases), "curated": len(curated), "random": len(rnd)},
            "sets": idx,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Unicode identifier fixtures")
    lines.append("")
    lines.append("Reference unicode identifier fixtures for Vitte.")
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
    lines.append("- Ident extraction uses a conservative unicode word matcher (regex).")
    lines.append("- NFC/NFD are reported to detect normalization-sensitive behavior.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Unicode identifier fixtures index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- stress: `{index.get('stress',0)}`")
    lines.append("")

    lines.append("| id | name | kind | path | occurrences | unique | tags | description |")
    lines.append("|---|---|---|---|---:|---:|---|---|")
    for s in index.get("sets", []):
        c = s.get("counts", {})
        lines.append(
            "| `{}` | `{}` | `{}` | `{}` | {} | {} | `{}` | {} |".format(
                s.get("id", ""),
                s.get("name", ""),
                s.get("kind", ""),
                s.get("path", ""),
                int(c.get("ident_occurrences", 0)),
                int(c.get("ident_unique", 0)),
                ",".join([str(t) for t in s.get("tags", [])]),
                s.get("description", ""),
            )
        )

    lines.append("")
    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_unicode_idents.py")
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
        _eprint(f"error: gen_unicode_idents failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())