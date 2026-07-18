#!/usr/bin/env python3
from __future__ import annotations

import re
import stat
import subprocess
from collections import Counter, deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "src" / "vitte" / "compiler"
OUT = ROOT / "AUDIT.md"

EXCLUDE_PARTS = {".git", "target", "pkgout", "build", ".aider.tags.cache.v4", ".steel-cache", ".vitte-cache"}
PLACEHOLDER_RE = re.compile(r"TODO|placeholder|stub|mock|panic|unimplemented", re.IGNORECASE)
USE_RE = re.compile(r"^\s*use\s+([A-Za-z0-9_/]+)")
PROC_RE = re.compile(r"^\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", re.MULTILINE)


def rel(path: Path) -> str:
    return str(path.relative_to(ROOT))


def visible_files() -> list[Path]:
    out: list[Path] = []
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if any(part in EXCLUDE_PARTS for part in path.relative_to(ROOT).parts):
            continue
        out.append(path)
    return sorted(out)


def executable_files(files: list[Path]) -> list[str]:
    result: list[str] = []
    for path in files:
        try:
            mode = path.stat().st_mode
        except OSError:
            continue
        if mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH):
            result.append(rel(path))
    return result


def compiler_modules() -> list[Path]:
    return sorted(COMPILER.rglob("*.vit"))


def module_id(path: Path) -> str:
    return str(path.relative_to(ROOT / "src").with_suffix("")).replace("\\", "/")


def is_test_or_fixture(path: Path) -> bool:
    parts = path.relative_to(COMPILER).parts
    return "tests" in parts or "frontend_fixtures" in parts


def dependency_graph(modules: list[Path]) -> tuple[dict[str, Path], dict[str, set[str]]]:
    by_id = {module_id(path): path for path in modules}
    deps: dict[str, set[str]] = {mid: set() for mid in by_id}
    for mid, path in by_id.items():
        text = path.read_text(encoding="utf-8", errors="replace")
        for line in text.splitlines():
            match = USE_RE.match(line)
            if not match:
                continue
            dep = match.group(1)
            if dep in by_id:
                deps[mid].add(dep)
    return by_id, deps


def reachable_from(roots: list[str], deps: dict[str, set[str]]) -> set[str]:
    seen: set[str] = set()
    q: deque[str] = deque(root for root in roots if root in deps)
    while q:
        cur = q.popleft()
        if cur in seen:
            continue
        seen.add(cur)
        for nxt in sorted(deps.get(cur, ())):
            if nxt not in seen:
                q.append(nxt)
    return seen


def placeholder_hits(files: list[Path]) -> list[str]:
    hits: list[str] = []
    for path in files:
        if path.suffix not in {".vit", ".vitl", ".py", ".sh", ".md", ".seed"}:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for lineno, line in enumerate(text.splitlines(), 1):
            if PLACEHOLDER_RE.search(line):
                hits.append(f"{rel(path)}:{lineno}: {line.strip()}")
    return hits


def procs_never_called(modules: list[Path]) -> list[str]:
    texts = {path: path.read_text(encoding="utf-8", errors="replace") for path in modules if not is_test_or_fixture(path)}
    joined = "\n".join(texts.values())
    call_counts = Counter(re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(", joined))
    candidates: list[str] = []
    for path, text in texts.items():
        for match in PROC_RE.finditer(text):
            name = match.group(1)
            if name in {"main", "version_text", "banner_text"}:
                continue
            if call_counts[name] <= 1:
                candidates.append(f"{rel(path)}: {name}")
    return candidates


def tests_that_verify_nothing(modules: list[Path]) -> list[str]:
    weak: list[str] = []
    for path in modules:
        if "/tests/" not in f"/{rel(path)}":
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for match in re.finditer(r"proc\s+(test_[A-Za-z0-9_]+)\s*\([^)]*\)\s*->\s*bool\s*\{", text):
            start = match.end()
            nxt = text.find("\nproc ", start)
            body = text[start : nxt if nxt >= 0 else len(text)]
            has_signal = any(token in body for token in ["==", "!=", ">=", "<=", ">", "<", "find(", "len(", ".valid", "if not", "give false"])
            if "give true" in body and not has_signal:
                weak.append(f"{rel(path)}: {match.group(1)}")
        if "proc main(args: list[string]) -> int" in text and "run_all_tests()" not in text and "set failures" not in text:
            weak.append(f"{rel(path)}: main does not dispatch run_all_tests")
    return weak


def run(args: list[str]) -> tuple[int, str]:
    proc = subprocess.run(args, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, errors="replace")
    return proc.returncode, proc.stdout.strip()


def codeblock(lines: list[str], limit: int | None = None) -> str:
    shown = lines if limit is None else lines[:limit]
    extra = "" if limit is None or len(lines) <= limit else f"\n... ({len(lines) - limit} more)"
    return "```text\n" + "\n".join(shown) + extra + "\n```"


def main() -> int:
    files = visible_files()
    modules = compiler_modules()
    all_file_count = sum(1 for path in ROOT.rglob("*") if path.is_file())
    by_id, deps = dependency_graph(modules)
    all_reachable = reachable_from(["vitte/compiler/main"], deps)
    real_roots = ["vitte/compiler/driver/compile", "vitte/compiler/driver/pipeline", "vitte/compiler/backend/pipeline"]
    real_reachable = reachable_from(real_roots, deps)
    non_test_ids = {module_id(path) for path in modules if not is_test_or_fixture(path)}
    dead_by_real_pipeline = sorted(non_test_ids - real_reachable)

    real_audit_rc, real_audit_out = run(["python3", "tools/compiler_real_pipeline_audit.py"])
    help_rc, help_out = run(["./bin/vittec", "--help"])
    check_pos_rc, check_pos_out = run(["./bin/vittec", "check", "tests/golden/frontend/fixtures/hello_min.vit"])
    build_target_rc, build_target_out = run(["./bin/vittec", "build", "tests/golden/frontend/fixtures/hello_min.vit", "-o", "target/audit_hello"])
    run_rc, run_out = run(["./bin/vittec", "run", "tests/golden/frontend/fixtures/hello_min.vit"])
    test_rc, test_out = run(["./bin/vittec", "test", "tests/golden/frontend/fixtures/hello_min.vit"])

    placeholders = placeholder_hits(files)
    uncalled = procs_never_called(modules)
    weak_tests = tests_that_verify_nothing(modules)
    source_main_wired = "runtime_dispatch=wired" in real_audit_out

    critical_absent = [
        "Command form `vittec build file.vit -o out` is not supported by the current binary.",
        "Commands `run` and `test` are not exposed by the current binary.",
        "Driver CLI helpers exist in src/vitte/compiler/driver/cli.vit but are not the runtime process entry.",
        "Diagnostics session modules exist, but the command path does not create a first-class session object.",
        "MIR optimisation is represented in modules/tests, but the compile path uses MIR validation before IR lowering, not a full optimisation pipeline.",
        "Object emission is pseudo text (`pseudo-object` / `elf-pseudo`), not a machine object file.",
        "Linker creates `vitte-bootstrap-artifact`, not a real native linked binary.",
        "The seed-generated compiler still carries a transition bridge; autonomous native self-hosting is not complete.",
    ]
    if not source_main_wired:
        critical_absent = ["compiler.vit main(args) does not dispatch the CLI; it still returns 0."] + critical_absent

    graph = """```mermaid
flowchart TD
  A[source .vit] --> B[frontend_run]
  B --> C[lexer + parser]
  C --> D[AST / FrontendOutput]
  D --> E[lower_ast_to_hir]
  E --> F[validate_hir]
  F --> G[run_sema_hir]
  G --> H[run_typeck_hir]
  H --> I[analysis_run_prechecked / borrowck]
  H --> J[lower_hir_to_mir]
  J --> K[validate_mir]
  K --> L[lower_mir_as_ir_module]
  L --> M[verify_unit]
  M --> N[run_codegen_x86_64 / run_codegen_llvm_with_profile]
  N --> O[pseudo object text today]
  O --> P[link_ir_unit_with_kind]
  P --> Q[LinkArtifact today]
  Q -. missing .-> R[native binary]
```"""

    lines = [
        "# AUDIT - Vitte compiler real pipeline",
        "",
        "Date: 2026-05-21",
        "",
        "## Verdict court",
        "",
        "Le depot contient beaucoup de pipeline compiler reel (frontend, HIR, sema, typeck, MIR, IR, codegen, link artifact), mais le binaire actuel n'est pas encore un compilateur natif complet au sens `source -> objet machine -> linker -> executable`.",
        "",
        "Le point d'entree source est bien `src/vitte/compiler/main.vit`. `main(args)` est cable cote source, mais le runtime CLI actif reste issu du payload genere par le seed tant que le bridge de transition n'est pas retire.",
        "",
        "## Executables trouves",
        "",
        f"Total fichiers visibles par `find . -type f`: {all_file_count}. Fichiers hors caches/build/target/pkgout: {len(files)}.",
        "",
        codeblock(executable_files(files), limit=180),
        "",
        "## Vrai main CLI",
        "",
        "- Entree compilateur canonique: `src/vitte/compiler/main.vit`.",
        f"- `src/vitte/compiler/main.vit::main(args)`: {'dispatcher source cable' if source_main_wired else 'placeholder encore detecte'}.",
        "- Commandes runtime observees par `./bin/vittec --help`:",
        "",
        codeblock([help_out]),
        "",
        "## Commandes build/check/run/test",
        "",
        f"- `vittec check tests/golden/frontend/fixtures/hello_min.vit`: rc={check_pos_rc}.",
        f"- `vittec build tests/golden/frontend/fixtures/hello_min.vit -o target/audit_hello`: rc={build_target_rc}.",
        f"- `vittec run tests/golden/frontend/fixtures/hello_min.vit`: rc={run_rc}.",
        f"- `vittec test tests/golden/frontend/fixtures/hello_min.vit`: rc={test_rc}.",
        "",
        "Sorties importantes:",
        "",
        codeblock([line for line in [check_pos_out, build_target_out, run_out, test_out] if line], limit=80),
        "",
        "## Graphe du pipeline reel",
        "",
        graph,
        "",
        "## Modules compiler/*",
        "",
        f"- Modules `.vit` sous `src/vitte/compiler`: {len(modules)}.",
        f"- Modules non-test atteignables depuis `main`: {len(all_reachable)} / {len(non_test_ids)}.",
        f"- Modules non-test atteignables depuis les racines de pipeline reel (`driver/compile`, `driver/pipeline`, `backend/pipeline`): {len(real_reachable & non_test_ids)} / {len(non_test_ids)}.",
        "",
        "## Modules morts ou non utilises par le pipeline reel",
        "",
        codeblock(dead_by_real_pipeline, limit=220),
        "",
        "## Placeholders / stubs / mocks / unimplemented",
        "",
        codeblock(placeholders, limit=220),
        "",
        "## Fonctions candidates jamais appelees",
        "",
        "Heuristique: declaration `proc name(` dont le nom n'apparait qu'une seule fois dans les sources compiler non-test.",
        "",
        codeblock(uncalled, limit=260),
        "",
        "## Tests qui ne verifient probablement rien",
        "",
        "Heuristique: test bool qui rend `true` sans signal d'assertion, ou main de test qui ne lance pas `run_all_tests`.",
        "",
        codeblock(weak_tests if weak_tests else ["Aucun candidat faible detecte par l'heuristique."], limit=120),
        "",
        "## Modules critiques absents",
        "",
        "\n".join(f"- {item}" for item in critical_absent),
        "",
        "## Audit zero pipeline fictif",
        "",
        f"`python3 tools/compiler_real_pipeline_audit.py` rc={real_audit_rc}.",
        "",
        codeblock(real_audit_out.splitlines(), limit=120),
        "",
        "## Commandes utilisees",
        "",
        "```sh",
        "find . -type f",
        "grep -R \"TODO\\|placeholder\\|stub\\|mock\\|panic\\|unimplemented\" .",
        "rg -n \"TODO|placeholder|stub|mock|panic|unimplemented\" . --glob '!target/**' --glob '!pkgout/**' --glob '!build/**' --glob '!.git/**'",
        "python3 tools/compiler_real_pipeline_audit.py",
        "python3 tools/compiler_audit_report.py",
        "```",
        "",
    ]

    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"[compiler-audit-report] wrote {OUT.relative_to(ROOT)}")
    print(f"[compiler-audit-report] modules={len(modules)} dead_by_real_pipeline={len(dead_by_real_pipeline)} placeholders={len(placeholders)} uncalled_candidates={len(uncalled)} weak_tests={len(weak_tests)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
