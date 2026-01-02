#!/usr/bin/env python3
"""gen_project.py

Generate deterministic *project* corpora for Vitte tooling and benches.

Use-cases
- CLI benches (project discovery, manifests, globbing)
- pkg/muffin loaders (workspace + package graphs)
- parser benches (multi-file parsing)
- fmt benches (formatting multiple files)

This generator emits *append-friendly* project sets: prefer adding new sets over
modifying existing ones.

Outputs (under --out):
  projects/
    index.json
    README.md
    INDEX.md                       (optional)
    sets/
      <id>_<name>/
        meta.json
        files.json                 # list of files with sha256 + bytes
        root/
          ...                      # project tree

Notes
- Files are generated to be syntactically valid for Vitte core/phrase surface
  (blocks use `.end`).
- Manifests are emitted as Muffin `.muf` documents (workspace + package/target).
- The corpora is *tooling-oriented* (discovery + parsing) rather than a full
  standard library.

Exit codes:
- 0: success
- 2: usage / input error
- 3: IO/internal error

Examples:
  python3 gen_project.py --out ../generated/projects --seed 0
  python3 gen_project.py --out ../generated/projects --seed 1 --count 30 --modules 40 --stress 2 --emit-md
"""

from __future__ import annotations

import argparse
import hashlib
from dataclasses import dataclass
from pathlib import PurePosixPath, Path
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

from vitte_benchgen.rng import Rng
from vitte_benchgen.util import Logger, dumps_json_stable
from vitte_benchgen.writer import StagingWriter, WriteOptions


# -----------------------------------------------------------------------------
# Case model
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class ProjectSet:
    id: str
    name: str
    description: str
    kind: str
    files: Tuple[Tuple[str, str], ...]  # (path, content)
    tags: Tuple[str, ...] = ()


def _case_id(seed: str, name: str) -> str:
    h = hashlib.sha256()
    h.update(seed.encode("utf-8"))
    h.update(b"\n")
    h.update(name.encode("utf-8"))
    return h.hexdigest()[:16]


def _mk_set(
    *,
    seed: str,
    name: str,
    description: str,
    kind: str,
    files: Sequence[Tuple[str, str]],
    tags: Sequence[str] = (),
) -> ProjectSet:
    sid = _case_id(seed, name)

    # Normalize paths to POSIX separators for stable metadata.
    norm: List[Tuple[str, str]] = []
    for p, c in files:
        pp = str(PurePosixPath(p))
        if pp.startswith("../") or pp.startswith("..\\") or pp.startswith("/"):
            raise ValueError(f"illegal relative root escape in path: {p}")
        norm.append((pp, c))

    # Stable ordering by path.
    norm.sort(key=lambda t: t[0])

    return ProjectSet(
        id=sid,
        name=name,
        description=description,
        kind=kind,
        files=tuple(norm),
        tags=tuple(tags),
    )


# -----------------------------------------------------------------------------
# Utilities
# -----------------------------------------------------------------------------


def _sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _uniq_keep(xs: Iterable[str]) -> List[str]:
    seen = set()
    out: List[str] = []
    for x in xs:
        if x in seen:
            continue
        seen.add(x)
        out.append(x)
    return out


def _safe_dir_name(ps: ProjectSet) -> str:
    name = ps.name.replace(" ", "_").replace("/", "_")
    return f"{ps.id}_{name}"


def _gen_ident(rng: Rng, n: int) -> str:
    alpha = "abcdefghijklmnopqrstuvwxyz"
    return "".join(alpha[rng.randint(0, len(alpha) - 1)] for _ in range(n))


def _emit_vitte_fn_main(body_lines: Sequence[str]) -> str:
    out: List[str] = ["fn main()"]
    for l in body_lines:
        out.append("  " + l)
    out.append(".end")
    return "\n".join(out) + "\n"


# -----------------------------------------------------------------------------
# Muffin `.muf` minimal templates
# -----------------------------------------------------------------------------


def _muf_workspace_min(*, name: str, packages: Sequence[str]) -> str:
    # This is a pragmatic manifest for bench fixtures, not a fully strict spec.
    # Keep it readable and stable.
    lines: List[str] = []
    lines.append("muf 1")
    lines.append("")
    lines.append("workspace")
    lines.append(f"  name {name}")
    lines.append(".end")
    lines.append("")

    lines.append("packages")
    for p in packages:
        lines.append(f"  {p}")
    lines.append(".end")
    lines.append("")

    lines.append("profile")
    lines.append("  default")
    lines.append("    opt 2")
    lines.append("    debug true")
    lines.append("  .end")
    lines.append(".end")
    lines.append("")

    return "\n".join(lines) + "\n"


def _muf_package_min(*, name: str, src_dir: str, targets: Sequence[Tuple[str, str]]) -> str:
    lines: List[str] = []
    lines.append("muf 1")
    lines.append("")
    lines.append("package")
    lines.append(f"  name {name}")
    lines.append(f"  src {src_dir}")
    lines.append("  version 0.1.0")
    lines.append(".end")
    lines.append("")

    lines.append("targets")
    for tkind, entry in targets:
        lines.append(f"  {tkind}")
        lines.append(f"    entry {entry}")
        lines.append("  .end")
    lines.append(".end")
    lines.append("")

    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# Curated sets
# -----------------------------------------------------------------------------


def curated_sets(seed: str) -> List[ProjectSet]:
    sets: List[ProjectSet] = []

    # 1) Minimal single package
    files1: List[Tuple[str, str]] = []
    files1.append(("root/muffin.muf", _muf_package_min(name="app", src_dir="src", targets=[("program", "src/main.vitte")])))
    files1.append(("root/src/main.vitte", _emit_vitte_fn_main(["say \"hello\"", "ret 0"])))
    files1.append(("root/README.md", "# app\n\nMinimal project fixture.\n"))
    sets.append(
        _mk_set(
            seed=seed,
            name="single_package_min",
            description="Single package with a minimal main",
            kind="package",
            files=files1,
            tags=["muf", "core", "smoke"],
        )
    )

    # 2) Multi-module parsing surface
    files2: List[Tuple[str, str]] = []
    files2.append(("root/muffin.muf", _muf_package_min(name="mm", src_dir="src", targets=[("program", "src/main.vitte")])))

    mod_a = (
        "module mm/a\n"
        "type Id = i64\n"
        "struct Pair\n"
        "  a: i32\n"
        "  b: i32\n"
        ".end\n"
        "fn add(x: i32, y: i32) -> i32\n"
        "  ret x + y\n"
        ".end\n"
    )

    mod_b = (
        "module mm/b\n"
        "use mm/a\n"
        "fn run() -> i32\n"
        "  let z = add(40, 2)\n"
        "  ret z\n"
        ".end\n"
    )

    main2 = (
        "module mm/main\n"
        "use mm/b\n"
        "fn main()\n"
        "  let v = run()\n"
        "  ret v\n"
        ".end\n"
    )

    files2.append(("root/src/a.vitte", mod_a))
    files2.append(("root/src/b.vitte", mod_b))
    files2.append(("root/src/main.vitte", main2))
    files2.append(("root/tests/smoke.t.vitte", "fn t()\n  ret\n.end\n"))

    sets.append(
        _mk_set(
            seed=seed,
            name="multi_module_small",
            description="Small multi-module project with use/module declarations",
            kind="package",
            files=files2,
            tags=["muf", "module", "parse"],
        )
    )

    # 3) Workspace with two packages
    files3: List[Tuple[str, str]] = []
    files3.append(("root/muffin.muf", _muf_workspace_min(name="ws", packages=["packages/app", "packages/lib"])))

    files3.append(("root/packages/app/muffin.muf", _muf_package_min(name="app", src_dir="src", targets=[("program", "src/main.vitte")])))
    files3.append(
        (
            "root/packages/app/src/main.vitte",
            "module app/main\nuse lib/math\nfn main()\n  let x = add(1, 2)\n  ret x\n.end\n",
        )
    )

    files3.append(("root/packages/lib/muffin.muf", _muf_package_min(name="lib", src_dir="src", targets=[("library", "src/lib.vitte")])))
    files3.append(
        (
            "root/packages/lib/src/lib.vitte",
            "module lib/math\nfn add(a: i32, b: i32) -> i32\n  ret a + b\n.end\n",
        )
    )

    files3.append(("root/.gitignore", "# fixture\n/dist\n/out\n"))

    sets.append(
        _mk_set(
            seed=seed,
            name="workspace_two_packages",
            description="Workspace fixture with app + lib packages",
            kind="workspace",
            files=files3,
            tags=["workspace", "muf", "deps"],
        )
    )

    # 4) Unicode filenames and identifiers (tooling surface)
    files4: List[Tuple[str, str]] = []
    files4.append(("root/muffin.muf", _muf_package_min(name="unicode", src_dir="src", targets=[("program", "src/main.vitte")])))
    # Fix: avoid a trailing quote mistake by emitting correct path.
    files4[0] = ("root/muffin.muf", _muf_package_min(name="unicode", src_dir="src", targets=[("program", "src/main.vitte")]))

    files4.append(
        (
            "root/src/bonjour_école.vitte",
            "module unicode/bonjour\nfn salut() -> i32\n  let café = \"café\"\n  ret 0\n.end\n",
        )
    )
    files4.append(
        (
            "root/src/main.vitte",
            "module unicode/main\nuse unicode/bonjour\nfn main()\n  let x = salut()\n  ret x\n.end\n",
        )
    )
    files4.append(("root/docs/README_ß.md", "Unicode path fixture.\n"))

    sets.append(
        _mk_set(
            seed=seed,
            name="unicode_paths",
            description="Unicode filenames + identifiers for fs/glob/discovery",
            kind="package",
            files=files4,
            tags=["unicode", "muf", "fs"],
        )
    )

    # 5) Deep tree and long paths (bench surface for traversal)
    files5: List[Tuple[str, str]] = []
    files5.append(("root/muffin.muf", _muf_package_min(name="deep", src_dir="src", targets=[("program", "src/main.vitte")])))

    # Create a deep directory chain in src.
    deep_parts = ["d" + str(i) for i in range(0, 40)]
    deep_dir = "root/src/" + "/".join(deep_parts)
    files5.append((deep_dir + "/leaf.vitte", "fn leaf() -> i32\n  ret 1\n.end\n"))

    files5.append(
        (
            "root/src/main.vitte",
            "fn main()\n  let x = 0\n  ret x\n.end\n",
        )
    )

    sets.append(
        _mk_set(
            seed=seed,
            name="deep_tree",
            description="Deep nested directories for traversal/IO benches",
            kind="package",
            files=files5,
            tags=["fs", "stress", "deep"],
        )
    )

    return sets


# -----------------------------------------------------------------------------
# Random sets
# -----------------------------------------------------------------------------


def _rand_stmt(rng: Rng, vars_: Sequence[str]) -> str:
    v = rng.choice(list(vars_))
    k = rng.randint(0, 3)
    if k == 0:
        return f"set {v} = {v} + {rng.randint(0, 9)}"
    if k == 1:
        return f"set {v} = {rng.randint(-50, 50)}"
    if k == 2:
        u = rng.choice(list(vars_))
        return f"set {v} = {v} + {u}"
    return f"if {v} > {rng.randint(0, 9)}"


def _rand_module_source(rng: Rng, *, mod_path: str, fn_count: int, stress: int) -> str:
    lines: List[str] = []
    lines.append(f"module {mod_path}")

    # Optional type declarations
    if rng.random() < 0.30:
        tname = "T" + _gen_ident(rng, 6)
        lines.append(f"struct {tname}")
        for i in range(1 + stress):
            lines.append(f"  f{i}: i32")
        lines.append(".end")

    for fi in range(fn_count):
        fname = "f" + str(fi) + "_" + _gen_ident(rng, 6)
        lines.append(f"fn {fname}(a: i32, b: i32) -> i32")

        vars_ = ["x", "y", "z"]
        lines.append("  let x = a")
        lines.append("  let y = b")
        lines.append("  let z = 0")

        # statements
        stmt_count = 3 + stress * 8
        opened_ifs = 0
        for _ in range(stmt_count):
            s = _rand_stmt(rng, vars_)
            if s.startswith("if "):
                lines.append("  " + s)
                opened_ifs += 1
            else:
                lines.append("  " + s)

        lines.append("  ret x + y + z")
        for _ in range(opened_ifs):
            lines.append("  .end")

        lines.append(".end")

    return "\n".join(lines) + "\n"


def random_sets(seed: str, *, count: int, modules: int, stress: int, logger: Logger) -> List[ProjectSet]:
    if count <= 0:
        return []

    rng = Rng.from_seed(seed + ":projects:random")
    out: List[ProjectSet] = []

    for i in range(count):
        name = f"rand_{i:04d}"
        sid = _case_id(seed, name)

        pkg_name = "p" + _gen_ident(rng, 6)

        files: List[Tuple[str, str]] = []
        files.append(("root/muffin.muf", _muf_package_min(name=pkg_name, src_dir="src", targets=[("program", "src/main.vitte")])))

        # Choose module names.
        mcount = max(1, modules + rng.randint(-modules // 4 if modules > 4 else 0, modules // 4 if modules > 4 else 0))
        mnames = [f"{pkg_name}/m{j}_{_gen_ident(rng, 5)}" for j in range(mcount)]
        mnames = _uniq_keep(mnames)

        # Emit modules.
        for j, mp in enumerate(mnames):
            src = _rand_module_source(rng, mod_path=mp, fn_count=1 + (stress % 3), stress=stress)
            files.append((f"root/src/mod_{j:03d}.vitte", src))

        # main imports a subset to force `use` parsing.
        use_n = min(len(mnames), 1 + stress * 4, 16)
        picks = [mnames[k] for k in range(0, use_n)]

        main_lines: List[str] = []
        main_lines.append(f"module {pkg_name}/main")
        for mp in picks:
            main_lines.append(f"use {mp}")

        main_lines.append("fn main()")
        main_lines.append("  let a = 1")
        main_lines.append("  let b = 2")
        main_lines.append("  let s = 0")
        main_lines.append("  for i in 0..10")
        main_lines.append("    set s = s + i")
        main_lines.append("  .end")
        main_lines.append("  ret s + a + b")
        main_lines.append(".end")

        files.append(("root/src/main.vitte", "\n".join(main_lines) + "\n"))

        # Add some non-source files (discovery/glob)
        files.append(("root/.gitignore", "# fixture\n/dist\n/out\n*.tmp\n"))
        files.append(("root/docs/notes.txt", "fixture project set\n"))

        out.append(
            ProjectSet(
                id=sid,
                name=name,
                description=f"Random project set: modules={len(mnames)}, stress={stress}",
                kind="package",
                files=tuple(sorted(((str(PurePosixPath(p)), c) for p, c in files), key=lambda t: t[0])),
                tags=("random", "muf", "parse", "fs"),
            )
        )

    logger.debug("projects: random", "count", len(out), "modules", modules, "stress", stress)
    return out


# -----------------------------------------------------------------------------
# Emission
# -----------------------------------------------------------------------------


def emit(out_dir: Path, *, seed: str, count: int, modules: int, stress: int, emit_md: bool, logger: Logger) -> None:
    curated = curated_sets(seed)
    rnd = random_sets(seed, count=count, modules=modules, stress=stress, logger=logger)

    sets = curated + rnd

    # Stable ordering: curated first, then random by name.
    sets.sort(key=lambda s: ("random" in s.tags, s.name, s.id))

    logger.info(
        "projects: emit",
        "curated",
        len(curated),
        "random",
        len(rnd),
        "total",
        len(sets),
        "out",
        str(out_dir),
    )

    opts = WriteOptions(normalize_lf=True, strip_trailing_ws=False, enforce_root=True)

    with StagingWriter(out_dir, logger=logger, opts=opts) as sw:
        w = sw.writer
        assert w is not None

        index_sets: List[Dict[str, Any]] = []

        for s in sets:
            d = _safe_dir_name(s)
            base = f"sets/{d}"

            # Write project tree under base/root/
            file_rows: List[Dict[str, Any]] = []
            total_bytes = 0

            for rel, content in s.files:
                # rel already includes "root/..." to keep projects isolated.
                path = f"{base}/{rel}"
                w.write_text(path, content)

                b = content.encode("utf-8", errors="replace")
                total_bytes += len(b)
                file_rows.append(
                    {
                        "path": rel,
                        "sha256": _sha256_bytes(b),
                        "bytes": len(b),
                        "lines": content.count("\n"),
                    }
                )

            files_json = {
                "set": {"id": s.id, "name": s.name},
                "counts": {"files": len(file_rows), "bytes": total_bytes},
                "files": sorted(file_rows, key=lambda r: r["path"]),
            }
            w.write_text(f"{base}/files.json", dumps_json_stable(files_json, indent=2) + "\n")

            meta = {
                "id": s.id,
                "name": s.name,
                "description": s.description,
                "kind": s.kind,
                "tags": list(s.tags),
                "counts": {"files": len(file_rows), "bytes": total_bytes},
                "root": f"{base}/root",
            }
            w.write_text(f"{base}/meta.json", dumps_json_stable(meta, indent=2) + "\n")

            index_sets.append(
                {
                    "id": s.id,
                    "name": s.name,
                    "kind": s.kind,
                    "path": base,
                    "root": f"{base}/root",
                    "tags": list(s.tags),
                    "description": s.description,
                    "counts": {"files": len(file_rows), "bytes": total_bytes},
                }
            )

        index: Dict[str, Any] = {
            "tool": "gen_project.py",
            "seed": seed,
            "count": count,
            "modules": modules,
            "stress": stress,
            "contract": {"name": "vitte.projects.v1", "version": 1},
            "counts": {
                "total": len(sets),
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
    lines.append("# Projects corpus")
    lines.append("")
    lines.append("Deterministic project trees for Vitte tooling, benches, and regression tests.")
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
    lines.append("- Keep set directory names stable once published.")
    lines.append("- All paths are emitted under each set's `root/` directory.")
    lines.append("")

    return "\n".join(lines) + "\n"


def _render_index_md(index: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Projects corpus index")
    lines.append("")
    lines.append(f"- seed: `{index.get('seed','')}`")
    lines.append(f"- count: `{index.get('count',0)}`")
    lines.append(f"- modules: `{index.get('modules',0)}`")
    lines.append(f"- stress: `{index.get('stress',0)}`")
    lines.append("")

    lines.append("| id | name | kind | path | files | bytes | tags | description |")
    lines.append("|---|---|---|---|---:|---:|---|---|")
    for s in index.get("sets", []):
        cid = str(s.get("id", ""))
        name = str(s.get("name", ""))
        kind = str(s.get("kind", ""))
        path = str(s.get("path", ""))
        tags = ",".join([str(t) for t in s.get("tags", [])])
        counts = s.get("counts", {})
        nfiles = int(counts.get("files", 0))
        nbytes = int(counts.get("bytes", 0))
        desc = str(s.get("description", ""))
        lines.append(f"| `{cid}` | `{name}` | `{kind}` | `{path}` | {nfiles} | {nbytes} | `{tags}` | {desc} |")

    lines.append("")
    return "\n".join(lines) + "\n"


# -----------------------------------------------------------------------------
# CLI
# -----------------------------------------------------------------------------


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(prog="gen_project.py")
    ap.add_argument("--out", required=True, help="Output directory")
    ap.add_argument("--seed", default="0", help="Deterministic seed")
    ap.add_argument("--count", type=int, default=0, help="Number of random sets")
    ap.add_argument("--modules", type=int, default=20, help="Approx module count per random set")
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
    if ns.modules <= 0:
        _eprint("error: --modules must be > 0")
        return 2
    if ns.stress < 0:
        _eprint("error: --stress must be >= 0")
        return 2

    log = Logger(level="debug" if ns.verbose else "info", quiet=bool(ns.quiet))

    try:
        emit(
            out_dir,
            seed=str(ns.seed),
            count=int(ns.count),
            modules=int(ns.modules),
            stress=int(ns.stress),
            emit_md=bool(ns.emit_md),
            logger=log,
        )
    except Exception as e:
        _eprint(f"error: gen_project failed: {e}")
        return 3

    return 0


def _eprint(*a: object) -> None:
    import sys

    print(*a, file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())
