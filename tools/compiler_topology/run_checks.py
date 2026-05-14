#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "src" / "vitte" / "compiler"

MODULES = [
    "abi","arena","ast","ast_ir","ast_lowering","ast_passes","ast_pretty","attr_parsing","baked_icu_data",
    "borrowck","builtin_macros","codegen_cranelift","codegen_gcc","codegen_llvm","codegen_ssa",
    "const_eval","data_structures","driver","driver_impl","error_codes","error_messages","errors",
    "expand","feature","fs_util","graphviz","hashes","hir","hir_analysis","hir_id","hir_pretty","hir_typeck",
    "incremental","index","index_macros","infer","interface","lexer","lint","lint_defs","llvm","log","macros",
    "metadata","middle","mir_build","mir_dataflow","mir_transform","monomorphize","next_trait_solver",
    "parse","parse_format","passes","pattern_analysis","privacy","proc_macro","public","public_bridge",
    "query_impl","resolve","sanitizers","serialize","session","span","symbol_mangling","target","thread_pool",
    "trait_selection","traits","transmute","ty_utils","type_ir","type_ir_macros","windows_rc",
]

REQUIRED_PACK = [
    "README.vitl",
    "manifest.vit",
    "contracts.vit",
    "diagnostics.vit",
    "metrics.vit",
    "pipeline.vit",
]


def fail(msg: str) -> int:
    print(f"[compiler-topology][error] {msg}", file=sys.stderr)
    return 1


def main() -> int:
    checker = ROOT / "bin" / "vitte"
    if not checker.exists():
        return fail("missing checker: bin/vitte")

    for name in MODULES:
        d = BASE / name
        if not d.exists() or not d.is_dir():
            return fail(f"missing top-level module dir: src/vitte/compiler/{name}")

        # Legacy profile compatibility, but now we require full per-module pack.
        mod_file = d / "mod.vit"
        if mod_file.exists():
            txt = mod_file.read_text(encoding="utf-8")
            if "rustc_" in txt or "rustc::" in txt:
                return fail(f"forbidden rustc marker in {mod_file.relative_to(ROOT)}")
            subprocess.run([str(checker), "check", str(mod_file)], check=True, cwd=ROOT)
        else:
            alt = d / "component_profile.vit"
            if not alt.exists():
                return fail(f"missing module file for {name}: mod.vit or component_profile.vit")
            subprocess.run([str(checker), "check", str(alt)], check=True, cwd=ROOT)

        for rel in REQUIRED_PACK:
            p = d / rel
            if not p.exists():
                return fail(f"missing required module file: {p.relative_to(ROOT)}")
            txt = p.read_text(encoding="utf-8")
            if "rustc_" in txt or "rustc::" in txt:
                return fail(f"forbidden rustc marker in {p.relative_to(ROOT)}")
            subprocess.run([str(checker), "check", str(p)], check=True, cwd=ROOT)

        api_file = d / "api.vit"
        if api_file.exists():
            subprocess.run([str(checker), "check", str(api_file)], check=True, cwd=ROOT)

        smoke = d / "tests" / "smoke.vit"
        if smoke.exists():
            subprocess.run([str(checker), "check", str(smoke)], check=True, cwd=ROOT)
        fixtures = d / "tests" / "fixtures.vit"
        if not fixtures.exists():
            return fail(f"missing fixture file: {fixtures.relative_to(ROOT)}")
        subprocess.run([str(checker), "check", str(fixtures)], check=True, cwd=ROOT)

    catalog_mod = BASE / "catalog" / "mod.vit"
    catalog_smoke = BASE / "catalog" / "tests" / "smoke.vit"
    for p in (catalog_mod, catalog_smoke):
        if not p.exists():
            return fail(f"missing file: {p.relative_to(ROOT)}")
        subprocess.run([str(checker), "check", str(p)], check=True, cwd=ROOT)

    print(f"[compiler-topology] checks passed ({len(MODULES)} modules)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
