#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "release_readiness_28_58.json"


REQS = [
    (28, "cascade_root_cause", ["src/vitte/compiler/tests/diagnostic_snapshot_tests.vit"], ["test_cascade_diagnostics_link_to_root", "root_cause"]),
    (29, "recovery_after_error", ["tests/grammar/recovery_manifest.txt", "docs/book/grammar/scripts/validate_examples.py"], ["recovery"]),
    (30, "ci_requires_suggestion_and_explanation", ["tools/check_diagnostic_catalog.py", "tools/check_suggestion_quality.py", "tools/check_stdlib_diagnostic_contract.py", "Makefile"], ["suggestion-quality", "diagnostic_with_explanation"]),
    (31, "vitte_explain_knowledge_base", ["src/vitte/compiler/driver/compiler.vit", "tools/explain_snapshots.sh", "tests/diagnostics/check_explain_cli.py"], ["vitte explain CODE", "diagnostic_explain_text_lang"]),
    (32, "invalid_and_fixed_examples_per_code", ["tools/generate_diagnostic_catalog_snapshots.py", "tests/diagnostics/catalog/parser.catalog.json"], ["valid", "invalid"]),
    (33, "beginner_expert_ide_profiles", ["src/vitte/compiler/diagnostics/diagnostic.vit", "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit", "tools/build_docs_site.py"], ["ide_render_config", "terminal_render_config", "beginner"]),
    (34, "rust_like_terminal_colors", ["src/vitte/compiler/tests/diagnostic_snapshot_tests.vit", "src/vitte/compiler/infrastructure/diagnostics/colors.vit"], ["rust_like", "Color"]),
    (35, "stable_versioned_json", ["src/vitte/compiler/diagnostics/json.vit", "src/vitte/compiler/diagnostics/diagnostic.vit"], ["schema_version", "diagnostic_schema_version"]),
    (36, "complete_lsp_codeaction_disabled", ["src/vitte/compiler/diagnostics/lsp.vit", "tools/lsp/run_checks.py"], ["disabled_reason", "CodeAction"]),
    (37, "public_diagnostics_api_frozen", ["src/vitte/compiler/diagnostics/diagnostic.vit", "tools/check_diagnostic_schema.py"], ["DIAGNOSTIC_PUBLIC_API_STATUS", "diagnostic_public_api_freeze_contract"]),
    (38, "automatic_diagnostic_changelog", ["tools/update_diagnostic_code_registry.py", "tools/diagnostic_catalog_data.py", "CHANGELOG.md"], ["changelog", "diagnostic"]),
    (39, "default_english_and_validated_locales", ["tools/check_diagnostics_locales.py", "locales/en/diagnostics.ftl", "locales/fr/diagnostics.ftl"], ["supported_locale_codes", "en"]),
    (40, "ci_rejects_vague_terms", ["tools/check_suggestion_quality.py", "tools/check_diagnostic_catalog.py"], ["VAGUE_PATTERNS", "forbidden vague term"]),
    (41, "stdlib_core_no_allocation", ["src/vitte/stdlib/core.vitl", "tools/strict_core_guard_test.sh", "tools/lint_core_forbidden_syntax.py"], ["core"]),
    (42, "alloc_collections_complete", ["src/vitte/stdlib/alloc", "src/vitte/stdlib/collections"], ["vec", "string", "hashmap", "btree", "deque"]),
    (43, "std_system_modules_complete", ["src/vitte/stdlib/std"], ["fs", "io", "path", "env", "process", "time", "thread", "sync", "net"]),
    (44, "std_testing_bench_log_cli", ["src/vitte/stdlib/std/testing.vitl", "src/vitte/stdlib/std/bench.vitl", "src/vitte/stdlib/std/log.vitl", "src/vitte/stdlib/std/cli.vitl"], ["bench", "log", "cli"]),
    (45, "serialization_json_csv_base64_uuid_semver", ["src/vitte/stdlib/json", "src/vitte/stdlib/encoding", "tools/stdlib/run_checks.py"], ["csv", "base64", "uuid", "semver"]),
    (46, "unicode_generated_from_ucd", ["src/vitte/stdlib/generated/unicode_tables.vitl", "tools/stdlib/generate_unicode_tables.py"], ["UCD", "GENERATED_UNICODE_VERSION"]),
    (47, "fuzz_utf8_path_json_csv_url", ["tools/parser_lexer_fuzz_smoke.py", "tools/stdlib/run_checks.py"], ["utf", "path", "json", "csv", "url"]),
    (48, "stdlib_benchmarks_regression_thresholds", ["tools/stdlib/run_checks.py", "data/profiling/baseline/bench_results.csv"], ["benchmark", "regression"]),
    (49, "generated_api_docs_from_sources", ["tools/stdlib/generate_api_docs.py", "tools/generate_stdlib_reference_pages.py"], ["source", "API"]),
    (50, "executable_examples_each_public_module", ["tools/build_examples_matrix.sh", "examples", "tools/quickstart_check.sh"], ["examples"]),
    (51, "packaging_all_platforms", ["scripts/build-linux-debs.sh", "scripts/build-bsd-installers.sh", "scripts/build-macos-installers.sh", "scripts/build-solaris-package.sh", "scripts/build-windows-installer.sh"], ["pkg"]),
    (52, "reproducible_installers", ["tools/scripts_build_reproducibility_test.sh", "tools/scripts_build_checksum_fallback_test.sh"], ["reproducible"]),
    (53, "sbom_checksums_manifest_per_artifact", ["scripts/build-all-installers.sh", "scripts/verify-installers.sh"], ["SBOM", "sha256", "manifest"]),
    (54, "signature_notarization", ["scripts/build-macos-installers.sh"], ["SIGN", "NOTARIZE"]),
    (55, "simulated_install_vitte_help", ["tools/scripts_build_arch_matrix_test.sh", "scripts/verify-installers.sh"], ["--help"]),
    (56, "installer_arch_support", ["scripts/package-matrix.sh", "data/ffi/abi/target_abi_v1.json"], ["x86_64", "arm64", "riscv64", "i386"]),
    (57, "windows_xp_to_11", ["scripts/build-windows-installer.sh", "toolchain/scripts/package/windows/vitte-installer.nsi"], ["XP", "11"]),
    (58, "bsd_full_family", ["scripts/build-bsd-installers.sh"], ["FreeBSD", "OpenBSD", "NetBSD", "DragonFly"]),
]


def read_evidence(paths: list[str]) -> str:
    chunks: list[str] = []
    for item in paths:
        path = ROOT / item
        if path.is_file():
            chunks.append(path.read_text(encoding="utf-8", errors="replace"))
        elif path.is_dir():
            names = "\n".join(p.name for p in path.rglob("*") if p.is_file())
            chunks.append(names)
    return "\n".join(chunks)


def main() -> int:
    failures: list[str] = []
    rows: list[dict[str, object]] = []
    for req_id, name, evidence, needles in REQS:
        missing = [item for item in evidence if not (ROOT / item).exists()]
        text = read_evidence(evidence)
        absent = [needle for needle in needles if needle.lower() not in text.lower()]
        if missing:
            failures.append(f"{req_id} {name}: missing evidence {', '.join(missing)}")
        if absent:
            failures.append(f"{req_id} {name}: missing proof terms {', '.join(absent)}")
        rows.append({
            "id": req_id,
            "name": name,
            "evidence": evidence,
            "required_terms": needles,
            "missing_evidence": missing,
            "missing_terms": absent,
            "covered": not missing and not absent,
        })
    payload = {
        "schema": "vitte.release_readiness_28_58",
        "schema_version": "1.0.0",
        "status": "pass" if not failures else "fail",
        "requirements": rows,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[release-28-58][error] {failure}", file=sys.stderr)
        print(f"[release-28-58] report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[release-28-58] OK requirements=28-58 report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
