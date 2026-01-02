

#!/usr/bin/env python3
"""gen_paths.py

Generate path/string corpora for Vitte benches, CLI, and filesystem edge-cases.

Why
- Cross-platform path handling is a recurring source of bugs:
  - separators (/ vs \\)
  - drive letters / UNC
  - case sensitivity
  - reserved names (Windows)
  - long paths and deep nesting
  - traversal (..), dot segments
  - Unicode normalization and confusables

This generator emits deterministic corpora that can be reused by:
- CLI args_matrix tests
- pal/fs unit tests
- bench inputs for path normalization / canonicalization

Outputs (under --out):
  paths/
    index.json
    README.md
    INDEX.md                      (optional)
    sets/
      <id>_<name>/
        paths.txt                 # one path per line
        paths.json                # stable JSON list
        vitte_paths.vitte         # Vitte source embedding strings
        meta.json

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_paths.py --out ../generated/paths --seed 0
  python3 gen_paths.py --out ../generated/paths --seed 1 --count 20 --size 200 --emit-md
"""

from __future__ import annotations

import argparse
import hashlib
import unicodedata
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
    paths: Tuple[str, ...]
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
    paths: Sequence[str],
    tags: Sequence[str] = (),
) -> Case:
    cid = _case_id(seed, name)
    return Case(
        id=cid,
        name=name,
        description=description,
        kind=kind,
        paths=tuple(paths),
        tags=tuple(tags),
    )


# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------


def _uniq_keep(xs: Sequence[str]) -> List[str]:
    seen = set()
    out: List[str] = []
    for x in xs:
        if x in seen:
            continue
        seen.add(x)
        out.append(x)
    return out


def _escape_vitte_string(s: str) -> str:
    # Minimal escape set for embedding in Vitte source.
    return (
        s.replace("\\", "\\\\")
        .replace("\"", "\\\"")
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("\t", "\\t")
    )


def _emit_vitte_source(case: Case) -> str:
    lines: List[str] = []
    lines.append("fn main()")

    # chunk to keep file readable
    chunk = 16
    idx = 0
    ps = list(case.paths)
    for i in range(0, len(ps), chunk):
        for p in ps[i : i + chunk]:
            lines.append(f"  let p{idx} = \"{_escape_vitte_string(p)}\"")
            idx += 1

    lines.append("  ret 0")
    lines.append(".end")
    return "\n".join(lines) + "\n"


def _deep_posix_path(depth: int, seg_len: int = 8) -> str:
    seg = "a" * seg_len
    return "/" + "/".join([seg] * depth)


def _deep_win_path(depth: int, seg_len: int = 8) -> str:
    seg = "a" * seg_len
    return "C:\\" + "\\".join([seg] * depth)


def _win_reserved_names() -> List[str]:
    base = [
        "CON",
        "PRN",
        "AUX",
        "NUL",
    ]
    base += [f"COM{i}" for i in range(1, 10)]
    base += [f"LPT{i}" for i in range(1, 10)]
    # reserved with extensions
    out = []
    for b in base:
        out.append(b)
        out.append(b + ".txt")
        out.append(b.lower())
    return out


def _unicode_forms() -> List[Tuple[str, str]]:
    # pairs of visually similar or normalization-sensitive forms
    pairs: List[Tuple[str, str]] = []

    # composed vs decomposed (é)
    a = "café"
    b = unicodedata.normalize("NFD", a)
    pairs.append((a, b))

    # Å composed vs decomposed
    a2 = "Ångström"
    b2 = unicodedata.normalize("NFD", a2)
    pairs.append((a2, b2))

    # Greek mu vs micro sign
    pairs.append(("μ", "µ"))

    # Cyrillic a vs Latin a
    pairs.append(("a", "а"))  # Latin 'a' vs Cyrillic 'а'

    # fullwidth characters
    pairs.append(("ABC", "ＡＢＣ"))

    return pairs


def _gen_ident(rng: Rng, n: int) -> str:
    alpha = "abcdefghijklmnopqrstuvwxyz"
    return "".join(alpha[rng.randint(0, len(alpha) - 1)] for _ in range(n))


# -----------------------------------------------------------------------------
# Curated sets
# -----------------------------------------------------------------------------


def curated_cases(seed: str) -> List[Case]:
    rng = Rng.from_seed(seed + ":paths:curated")

    basic: List[str] = [
        "a",
        "a/b",
        "./a",
        "./a/./b",
        "a/..",
        "a/../b",
        "../a",
        "../../a",
        "a//b",
        "a///b",
        "/",
        "/tmp",
        "/tmp/..",
        "/tmp/./x",
    ]

    windows: List[str] = [
        "C:\\",
        "C:\\Windows\\System32",
        "C:\\Temp\\..\\x",
        "C:\\a\\.\\b",
        "C:\\a\\..\\b",
        "C:\\a\\b\\",
        "C:\\a\\b\\..\\..\\c",
        "\\\\server\\share",
        "\\\\server\\share\\dir\\file.txt",
        "\\?\\C:\\very\\long\\path",
    ]

    win_reserved = [f"C:\\{n}" for n in _win_reserved_names()]

    mixed: List[str] = [
        "a\\b",
        "a/\\b",
        "a\\/b",
        "C:/Windows/System32",
        "C:\\Windows/System32",
    ]

    # Long and deep paths
    deep: List[str] = [
        _deep_posix_path(10),
        _deep_posix_path(64),
        _deep_posix_path(200),
        _deep_win_path(10),
        _deep_win_path(64),
        _deep_win_path(200),
    ]

    # Edge segments
    segs: List[str] = [
        "a.",
        ".a",
        "a..",
        "..a",
        "a..b",
        "a...b",
        " ",
        "  ",
        "a b",
        "a\tb",
        "a\nb",
        "a\rb",
        "a\u0000b",  # embedded NUL marker (string surface)
    ]

    # Unicode normalization/confusables
    uni: List[str] = []
    for x, y in _unicode_forms():
        uni.append(x)
        uni.append(y)
        uni.append("/tmp/" + x)
        uni.append("/tmp/" + y)
        uni.append("C:\\Temp\\" + x)
        uni.append("C:\\Temp\\" + y)

    # Random-ish but deterministic to diversify
    rnd: List[str] = []
    for _ in range(64):
        d = rng.randint(1, 8)
        parts = [_gen_ident(rng, rng.randint(1, 12)) for _ in range(d)]
        if rng.random() < 0.2:
            # add dot segment
            parts.insert(rng.randint(0, len(parts)), "..")
        if rng.random() < 0.15:
            parts.insert(rng.randint(0, len(parts)), ".")
        p = "/".join(parts)
        if rng.random() < 0.25:
            p = "/" + p
        rnd.append(p)

    # Normalize list order: curated are stable concatenations
    basic = _uniq_keep(basic)
    windows = _uniq_keep(windows)
    mixed = _uniq_keep(mixed)
    deep = _uniq_keep(deep)
    segs = _uniq_keep(segs)
    uni = _uniq_keep(uni)
    rnd = _uniq_keep(rnd)

    return [
        _mk_case(
            seed=seed,
            name="basic_posix",
            description="Basic POSIX-like relative/absolute paths and dot segments",
            kind="posix",
            paths=basic,
            tags=["posix", "dot"],
        ),
        _mk_case(
            seed=seed,
            name="windows_paths",
            description="Windows drive/UNC/extended path forms",
            kind="windows",
            paths=windows + win_reserved,
            tags=["windows", "unc", "reserved"],
        ),
        _mk_case(
            seed=seed,
            name="mixed_separators",
            description="Mixed separators to test normalization",
            kind="mixed",
            paths=mixed,
            tags=["mixed"],
        ),
        _mk_case(
            seed=seed,
            name="deep_long",
            description="Deep/long paths to test limits",
            kind="stress",
            paths=deep,
            tags=["stress", "long"],
        ),
        _mk_case(
            seed=seed,
            name="segment_edges",
            description="Edge-case segments (spaces, control chars markers)",
            kind="segments",
            paths=segs,
            tags=["segments", "control"],
        ),
        _mk_case(
            seed=seed,
            name="unicode_forms",
            description="Unicode normalization and confusable forms",
            kind="unicode",
            paths=uni,
            tags=["unicode"],
        ),
        _mk_case(
            seed=seed,
            name="random_diverse",
            description="Deterministic random diverse POSIX-like paths",
            kind="posix",
            paths=rnd,
            tags=["random", "posix"],
        ),
    ]


# -----------------------------------------------------------------------------
# Random sets
# -----------------------------------------------------------------------------


def _rand_posix_path(rng: Rng) -> str:
    depth = rng.randint(0, 16)
    parts: List[str] = []
    for _ in range(depth):
        p = _gen_ident(rng, rng.randint(1, 16))
        if rng.random() < 0.08:
            p += "-" + _gen_ident(rng, rng.randint(1, 8))
        parts.append(p)

    if rng.random() < 0.15:
        parts.insert(rng.randint(0, len(parts) + 1), ".")
    if rng.random() < 0.15:
        parts.insert(rng.randint(0, len(parts) + 1), "..")

    s = "/".join(parts) if parts else ""
    if rng.random() < 0.25:
        s = "/" + s
    if rng.random() < 0.10:
        s = s + "/"
    if rng.random() < 0.05:
        s = s.replace("/", "//")
    return s or "."


def _rand_win_path(rng: Rng) -> str:
    # drive or UNC
    if rng.random() < 0.2:
        server = _gen_ident(rng, rng.randint(3, 10))
        share = _gen_ident(rng, rng.randint(3, 10))
        base = f"\\\\{server}\\{share}"
        depth = rng.randint(0, 10)
        parts = [_gen_ident(rng, rng.randint(1, 12)) for _ in range(depth)]
        return base + ("\\" + "\\".join(parts) if parts else "")

    drive = chr(ord("C") + rng.randint(0, 3))
    depth = rng.randint(0, 16)
    parts = [_gen_ident(rng, rng.randint(1, 12)) for _ in range(depth)]
    s = f"{drive}:\\" + "\\".join(parts)
    if rng.random() < 0.10:
        s += "\\"
    if rng.random() < 0.10 and parts:
        s = s.replace("\\", "/")
    return s


def random_cases(seed: str, *, count: int, size: int, logger: Logger) -> List[Case]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":paths:random")
    out: List[Case] = []

    for i in range(count):
        name = f"rand_{i:04d}"
        cid = _case_id(seed, name)

        ps: List[str] = []
        for _ in range(size):
            if rng.random() < 0.65:
                ps.append(_rand_posix_path(rng))
            else:
                ps.append(_rand_win_path(rng))

        ps = _uniq_keep(ps)

        out.append(
            Case(
                id=cid,
                name=name,
                description=f"Random mixed paths, size={len(ps)}",
                kind="mixed",
                paths=tuple(ps),
                tags=("random", "mixed"),
            )
        )

    logger.debug("paths: random", "count", len(out), "size", size)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def _safe_dir_name(case: Case) -> str:
    name = case.name.replace(" ", "_").replace("/", "_")
    return f"{case.id}_{name}"


def emit(out_dir: Path, *, seed: str, count: int, size: int, emit_md: bool, logger: Logger) -> None:
    curated = curated_cases(seed)
    rnd = random_cases(seed, count=count, size=size, logger=logger)

    cases = curated + rnd

    # stable ordering: curated first then random
    cases.sort(key=lambda c: ("random" in c.tags, c.name, c.id))

    logger.info("paths: emit", "curated", len(curated), "random", len(rnd), "total", len(cases), "out", str(out_dir))

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        index_sets: List[Dict[str, Any]] = []

        for c in cases:
            d = _safe_dir_name(c)
            base = f"sets/{d}"

            w.write_text(f"{base}/paths.txt", "\n".join(c.paths) + "\n")
            w.write_text(
                f"{base}/paths.json",
                dumps_json_stable({"kind": c.kind, "paths": list(c.paths)}, indent=2) + "\n",
            )
            w.write_text(f"{base}/vitte_paths.vitte", _emit_vitte_source(c))

            meta = {
                "id": c.id,
                "name": c.name,
                "description": c.description,
                "kind": c.kind,
                "tags": list(c.tags),
                "counts": {"paths": len(c.paths)},
            }
            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")

            index_sets.append(
                {
                    "id": c.id,
                    "name": c.name,
                    "kind": c.kind,
                    "path": base,
                    "tags": list(c.tags),
                    "description": c.description,
                    "counts": {"paths": len(c.paths)},
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_paths.py",
            "seed": seed,
            "count": count,
            "size": size,
            "contract": {"name": "vitte.paths.v1", "version": 1},
            "counts": {
                "total": len(cases),
                "curated": len(curated),
                "random": len(rnd),
            },
            "sets": index_sets,
        }

        w.write_text("index.json", dumps_json_stable(index, indent=2) + "\n")
        w.write_text("README.md", _render_readme(index))
        if emit_md:
            w.write_text("INDEX.md", _render_index_md(index))


def _render_readme(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Paths corpus")
    lines.append("")
    lines.append("Deterministic corpora for path parsing/normalization across platforms.")
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
    lines.append("- Prefer adding new sets instead of modifying existing ones.")
    lines.append("- Keep case directory names stable once published.")
    lines.append("- Some entries are *edge markers* (e.g., NUL) and may not be valid OS paths.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Paths corpus index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- size: `{index.get('size',0)}`")
    lines.append("")

    lines.append("| id | name | kind | path | tags | paths | description |")
    lines.append("|---|---|---|---|---|---:|---|")
    for s in index.get("sets", []):
        cid = str(s.get("id", ""))
        name = str(s.get("name", ""))
        kind = str(s.get("kind", ""))
        path = str(s.get("path", ""))
        tags = ",".join([str(t) for t in s.get("tags", [])])
        npaths = int(s.get("counts", {}).get("paths", 0))
        desc = str(s.get("description", ""))
        lines.append(f"| `{cid}` | `{name}` | `{kind}` | `{path}` | `{tags}` | {npaths} | {desc} |")

    lines.append("")
    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_paths.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--count", type=int, default=0, help="Number of random sets")
    ap.add_argument("--size", type=int, default=200, help="Paths per random set")
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
        _eprint(f"error: gen_paths failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())