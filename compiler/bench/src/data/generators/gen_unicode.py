

#!/usr/bin/env python3
"""gen_unicode.py

Generate deterministic *unicode* fixtures for Vitte.

This generator focuses on unicode content beyond identifiers:
- strings containing diverse scripts
- normalization (NFC/NFD) and combining marks
- emoji + variation selectors
- zero width joiners (ZWJ) sequences
- bidirectional (BiDi) control characters (included as data, not recommended)
- whitespace variants (NBSP, thin space)

The output includes reference metadata for each fixture so tools can validate
round-tripping and rendering behavior.

Outputs (under --out):
  unicode/
    index.json
    README.md
    INDEX.md                         (optional)
    sets/
      <id>_<name>/
        input.vitte
        expected.unicode.json         # reference unicode properties
        meta.json

Contract
- expected.unicode.json
  - contract: vitte.unicode.v1
  - strings: list of string samples extracted from input with:
      - text
      - codepoints
      - nfc/nfd
      - length (codepoints)
      - has_zwj / has_vs / has_bidi_controls
      - category_summary (bucketed by general category prefix)

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_unicode.py --out ../generated/unicode --seed 0
  python3 gen_unicode.py --out ../generated/unicode --seed 0 --count 40 --stress 2 --emit-md
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
# Unicode helpers
# -----------------------------------------------------------------------------


def _codepoints(s: str) -> List[str]:
    return [f"U+{ord(ch):04X}" for ch in s]


def _category_bucket(ch: str) -> str:
    cat = unicodedata.category(ch)
    return cat[:1] if cat else "?"


def _category_summary(s: str) -> Dict[str, int]:
    out: Dict[str, int] = {}
    for ch in s:
        b = _category_bucket(ch)
        out[b] = out.get(b, 0) + 1
    return out


def _has_any(s: str, cps: Sequence[int]) -> bool:
    st = set(cps)
    return any(ord(ch) in st for ch in s)


# ZWJ (U+200D)
_ZWJ = 0x200D

# Variation Selectors (U+FE00..U+FE0F)
_VS = list(range(0xFE00, 0xFE10))

# BiDi controls (a common subset)
_BIDI = [
    0x202A,  # LRE
    0x202B,  # RLE
    0x202D,  # LRO
    0x202E,  # RLO
    0x202C,  # PDF
    0x2066,  # LRI
    0x2067,  # RLI
    0x2068,  # FSI
    0x2069,  # PDI
]


# -----------------------------------------------------------------------------
# Extraction
# -----------------------------------------------------------------------------


# Extract string literals "..." with backslash escapes.
_RE_STRING = re.compile(r'\"(?:\\.|[^\"\\])*\"', re.UNICODE)


def extract_strings(src: str) -> List[str]:
    # Return unescaped raw literal text inside quotes (best-effort, stable).
    out: List[str] = []
    for m in _RE_STRING.finditer(src):
        lit = m.group(0)
        # Strip quotes
        inner = lit[1:-1]
        # Keep escapes as-is (do not interpret). This avoids semantic mismatch.
        out.append(inner)
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
    strings: Tuple[str, ...]
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
    strs = tuple(extract_strings(input_vitte))
    return Case(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        input_vitte=input_vitte,
        strings=strs,
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
            name="multiscript_strings",
            description="Strings containing diverse scripts",
            kind="core",
            input_vitte=(
                "fn main()\n"
                "  let a = \"café élève façade\"\n"
                "  let b = \"Привет мир\"\n"
                "  let c = \"漢字 東京 数据\"\n"
                "  let d = \"αβγ Δ μ π σ\"\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["strings", "scripts"],
        )
    )

    # Combining marks (NFD) included as literal characters.
    e_nfd = "e\u0301"  # e + combining acute
    a_dia = "a\u0308"  # a + combining diaeresis
    cases.append(
        _mk_case(
            seed=seed,
            name="combining_marks",
            description="Strings with combining marks (NFD sequences)",
            kind="core",
            input_vitte=(
                "fn main()\n"
                f"  let s = \"{e_nfd} {a_dia}\"\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["strings", "combining"],
        )
    )

    # Emoji with VS16 (U+FE0F) and ZWJ sequences.
    # Example: "woman technologist" often represented with ZWJ sequences.
    woman_tech = "\U0001F469\u200D\U0001F4BB"  # 👩‍💻
    heart = "\u2764\uFE0F"  # ❤️ (VS16)
    cases.append(
        _mk_case(
            seed=seed,
            name="emoji_zwj_vs",
            description="Emoji ZWJ sequences and variation selectors",
            kind="core",
            input_vitte=(
                "fn main()\n"
                f"  let a = \"{woman_tech}\"\n"
                f"  let b = \"{heart}\"\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["emoji", "zwj", "vs"],
        )
    )

    # Whitespace variants inside strings.
    nbsp = "\u00A0"  # non-breaking space
    thin = "\u2009"  # thin space
    cases.append(
        _mk_case(
            seed=seed,
            name="whitespace_variants",
            description="NBSP and thin spaces in strings",
            kind="core",
            input_vitte=(
                "fn main()\n"
                f"  let s = \"a{nbsp}b{thin}c\"\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["whitespace"],
        )
    )

    # BiDi controls included as literal chars in strings (dangerous in practice).
    rlo = "\u202E"  # Right-to-left override
    pdf = "\u202C"  # Pop directional formatting
    cases.append(
        _mk_case(
            seed=seed,
            name="bidi_controls",
            description="BiDi control characters inside a string literal",
            kind="core",
            input_vitte=(
                "fn main()\n"
                f"  let s = \"safe{rlo}evil{pdf}safe\"\n"
                "  ret 0\n"
                ".end\n"
            ),
            tags=["bidi"],
        )
    )

    return cases


# -----------------------------------------------------------------------------
# Random cases
# -----------------------------------------------------------------------------


def _rand_from_pool(rng: Rng, pool: Sequence[str]) -> str:
    return pool[rng.randint(0, len(pool) - 1)]


def _rand_unicode_string(rng: Rng, *, stress: int) -> str:
    latin = ["café", "élève", "façade", "naïve", "über"]
    cyr = ["Привет", "мир", "данные", "функция"]
    greek = ["αβγ", "Δ", "μ", "π", "σ"]
    cjk = ["漢字", "東京", "数据", "函数", "变量"]
    emoji = ["\U0001F680", "\U0001F525", "\U0001F4A1"]  # 🚀🔥💡
    comb = ["e\u0301", "a\u0308", "o\u0302"]

    chunks = [latin, cyr, greek, cjk, emoji, comb]

    n = 3 + rng.randint(0, 4 + stress)
    parts: List[str] = []
    for _ in range(n):
        parts.append(_rand_from_pool(rng, _rand_from_pool(rng, chunks)))

    # Insert separators sometimes
    sep_pool = [" ", "-", "_", "\u00A0", "\u2009"]
    sep = _rand_from_pool(rng, sep_pool)

    s = sep.join(parts)

    # Occasionally add VS16 or ZWJ
    if rng.random() < 0.12:
        s += "\u2764\uFE0F"  # ❤️
    if rng.random() < 0.08:
        s += "\U0001F469\u200D\U0001F4BB"  # 👩‍💻

    # Occasionally include BiDi controls
    if stress > 0 and rng.random() < 0.04:
        s = "safe" + "\u202E" + s + "\u202C" + "safe"

    return s


def random_cases(seed: str, *, count: int, stress: int, logger: Logger) -> List[Case]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":unicode:random")
    out: List[Case] = []

    for i in range(count):
        name = f"rand_{i:04d}"

        n_str = 4 + stress * 6
        strs = [_rand_unicode_string(rng, stress=stress) for _ in range(n_str)]

        lines: List[str] = []
        if rng.random() < 0.25:
            lines.append("# unicode strings")

        lines.append("fn main()")
        for j, s in enumerate(strs):
            # Keep escapes minimal; include raw unicode directly.
            lines.append(f"  let s{j} = \"{s}\"")

        lines.append("  ret 0")
        lines.append(".end")

        src = "\n".join(lines) + "\n"

        out.append(
            _mk_case(
                seed=seed,
                name=name,
                description=f"Random unicode string fixture {i} (stress={stress})",
                kind="core",
                input_vitte=src,
                tags=["random", "stress" if stress else "random"],
            )
        )

    logger.debug("unicode: random", "count", len(out), "stress", stress)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(c: Case) -> str:
    name = c.name.replace(" ", "_").replace("/", "_")
    return f"{c.id}_{name}"


def _string_props(s: str) -> Dict[str, Any]:
    return {
        "text": s,
        "codepoints": _codepoints(s),
        "nfc": unicodedata.normalize("NFC", s),
        "nfd": unicodedata.normalize("NFD", s),
        "length": len(s),
        "has_zwj": _has_any(s, [_ZWJ]),
        "has_vs": _has_any(s, _VS),
        "has_bidi_controls": _has_any(s, _BIDI),
        "category_summary": _category_summary(s),
    }


def _case_contract(c: Case) -> Dict[str, Any]:
    uniq: List[str] = []
    seen: set[str] = set()
    for s in c.strings:
        if s not in seen:
            uniq.append(s)
            seen.add(s)

    return {
        "contract": {"name": "vitte.unicode.v1", "version": 1},
        "case": {"id": c.id, "name": c.name, "kind": c.kind},
        "description": c.description,
        "counts": {"strings": len(c.strings), "strings_unique": len(uniq)},
        "strings": [_string_props(s) for s in uniq],
    }


def emit(out_dir: Path, *, seed: str, count: int, stress: int, emit_md: bool, logger: Logger) -> None:
    curated = curated_cases(seed)
    rnd = random_cases(seed, count=count, stress=stress, logger=logger)

    cases = curated + rnd
    cases.sort(key=lambda c: ("random" in c.tags, c.name, c.id))

    logger.info("unicode: emit", "curated", len(curated), "random", len(rnd), "total", len(cases), "out", str(out_dir))

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
            "tool": "gen_unicode.py",
            "seed": seed,
            "count": count,
            "stress": stress,
            "contract": {"name": "vitte.unicode.v1", "version": 1},
            "counts": {"total": len(cases), "curated": len(curated), "random": len(rnd)},
            "sets": idx,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Unicode fixtures")
    lines.append("")
    lines.append("Reference unicode fixtures focused on string literals and rendering-sensitive characters.")
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
    lines.append("- Strings are extracted from input and reported with NFC/NFD and codepoints.")
    lines.append("- Some fixtures include BiDi controls as data; handle carefully in UIs and logs.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Unicode fixtures index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- stress: `{index.get('stress',0)}`")
    lines.append("")

    lines.append("| id | name | kind | path | strings | unique | tags | description |")
    lines.append("|---|---|---|---|---:|---:|---|---|")
    for s in index.get("sets", []):
        c = s.get("counts", {})
        lines.append(
            "| `{}` | `{}` | `{}` | `{}` | {} | {} | `{}` | {} |".format(
                s.get("id", ""),
                s.get("name", ""),
                s.get("kind", ""),
                s.get("path", ""),
                int(c.get("strings", 0)),
                int(c.get("strings_unique", 0)),
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
    ap = argparse.ArgumentParser(prog="gen_unicode.py")
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
        _eprint(f"error: gen_unicode failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())