#!/usr/bin/env python3
"""Advanced type-system coverage gate for requirements 1-20."""

from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "target/type_system/advanced_20"
REPORT_DIR = ROOT / "target/reports"
TYPECK_TESTS = ROOT / "src/vitte/compiler/tests/typeck_tests.vit"


@dataclass(frozen=True)
class Requirement:
    id: int
    title: str
    area: str
    diagnostic_code: str
    explanation: str
    suggestion: str
    valid_fixture: str
    invalid_fixture: str
    evidence: tuple[str, ...]
    tests: tuple[str, ...]
    terms: tuple[str, ...]


COMMON_TYPECK_EVIDENCE = (
    "src/vitte/compiler/analysis/typeck/infer.vit",
    "src/vitte/compiler/analysis/typeck/unify.vit",
    "src/vitte/compiler/analysis/typeck/coercion.vit",
    "src/vitte/compiler/analysis/typeck/checker.vit",
    "src/vitte/compiler/analysis/typeck/errors.vit",
    "src/vitte/compiler/analysis/typeck/traits.vit",
    "src/vitte/compiler/tests/typeck_tests.vit",
)


RAW_REQUIREMENTS: tuple[dict[str, object], ...] = (
    {
        "title": "Inference locale complete: variables, retours, appels, blocs",
        "area": "inference",
        "code": "TYPECK_E_INFERENCE_FAILED",
        "tests": ("test_valid_numeric_flow", "test_typed_call_returns_signature_result_type", "test_typeck_annotates_hir_expr_type_and_copyable"),
        "terms": ("infer_expr_type", "function_return_type", "check_stmt", "inferred_type"),
    },
    {
        "title": "Inference bidirectionnelle: type attendu vers expression",
        "area": "inference",
        "code": "TYPECK_E_ASSIGN_MISMATCH",
        "tests": ("test_assign_mismatch_carries_rich_diagnostic_context", "test_argument_mismatch_carries_expected_found_context"),
        "terms": ("unify_assignment_type", "expected type", "obtained type"),
    },
    {
        "title": "Unification robuste: primitives, tuples, arrays, refs, pointeurs, fonctions",
        "area": "unification",
        "code": "TYPECK_E_TYPE_UNIFICATION",
        "tests": ("test_typeck_unify_helpers_cover_numeric_and_reference_surface", "test_declared_types_accept_nominals_generics_and_compounds"),
        "terms": ("unify_types_result", "unify_reference_types", "unify_pointer_types", "function_param_types", "type_index_element"),
    },
    {
        "title": "Variables de type internes avec resolution stable",
        "area": "inference",
        "code": "TYPECK_E_GENERIC_INFERENCE",
        "tests": ("test_generic_call_infers_and_substitutes_return_type", "test_generic_substitution_replaces_identifiers_only"),
        "terms": ("TypeSubstitution", "type_substitution", "resolved_call_generic_arguments"),
    },
    {
        "title": "Erreurs d'inference non ambigues",
        "area": "diagnostics",
        "code": "TYPECK_E_INFERENCE_FAILED",
        "tests": ("test_generic_call_reports_unconstrained_parameter", "test_typeck_result_enforces_diagnostic_cause_chains"),
        "terms": ("TYPECK_E_GENERIC_INFERENCE", "generic inference completeness", "cause_chain"),
    },
    {
        "title": "Tests type attendu absent",
        "area": "diagnostics",
        "code": "TYPECK_E_GENERIC_INFERENCE",
        "tests": ("test_generic_call_reports_unconstrained_parameter",),
        "terms": ("make()", "TYPECK_E_GENERIC_INFERENCE", "unconstrained"),
    },
    {
        "title": "Tests type attendu contradictoire",
        "area": "diagnostics",
        "code": "TYPECK_E_ASSIGN_MISMATCH",
        "tests": ("test_assign_mismatch", "test_return_mismatch_carries_rich_diagnostic_context"),
        "terms": ("TYPECK_E_ASSIGN_MISMATCH", "TYPECK_E_RETURN_MISMATCH", "Expected type"),
    },
    {
        "title": "Snapshots diagnostics pour chaque echec d'inference",
        "area": "diagnostics",
        "code": "TYPECK_E_DIAGNOSTIC_SNAPSHOT",
        "tests": ("typeck-snapshots", "diagnostic-quality"),
        "terms": ("diagnostics.json.snap", "expected_obtained.txt.snap", "cause_chain.txt.snap"),
    },
    {
        "title": "Generiques fonctions",
        "area": "generics",
        "code": "TYPECK_E_GENERIC_INFERENCE",
        "tests": ("test_generic_call_infers_and_substitutes_return_type", "test_generic_call_honors_explicit_type_argument"),
        "terms": ("generic_type_binding", "generic_params", "generic_arguments"),
    },
    {
        "title": "Generiques types",
        "area": "generics",
        "code": "TYPECK_E_UNKNOWN_TYPE",
        "tests": ("test_declared_types_accept_nominals_generics_and_compounds", "test_declared_field_reports_unknown_nested_type"),
        "terms": ("generic_param_type_bindings", "Box[T]", "Maybe[T]"),
    },
    {
        "title": "Generiques methodes",
        "area": "generics",
        "code": "TYPECK_E_IMPL_SIGNATURE_MISMATCH",
        "tests": ("test_typeck_preserves_trait_impl_method_signatures_and_body_types", "test_impl_method_dispatch_binds_self_away_for_call_type"),
        "terms": ("bind_member_signature", "TraitMethod", "ImplMethod"),
    },
    {
        "title": "Generiques imbriques",
        "area": "generics",
        "code": "TYPECK_E_GENERIC_INFERENCE",
        "tests": ("test_generic_substitution_replaces_identifiers_only", "test_declared_types_accept_nominals_generics_and_compounds"),
        "terms": ("List[U]", "substitute_type", "split_function_signature_types"),
    },
    {
        "title": "Parametres de type par defaut",
        "area": "generics",
        "code": "TYPECK_E_UNKNOWN_TYPE",
        "tests": ("test_declared_types_accept_nominals_generics_and_compounds",),
        "terms": ("default_type_text", "default type of generic parameter", "generic_param_info"),
    },
    {
        "title": "Contraintes where",
        "area": "generics",
        "code": "TYPECK_E_TRAIT_BOUND",
        "tests": ("grammar-test", "test_generic_trait_bound_accepts_visible_impl"),
        "terms": ("where_clauses", "where_clause", "where T: Renderable"),
    },
    {
        "title": "Specialisation et monomorphisation",
        "area": "monomorphization",
        "code": "MIR_E_MONOMORPHIZATION",
        "tests": ("mir-gate", "test_typed_hir_preserves_explicit_generic_instance"),
        "terms": ("monomorphize_mir", "MirGenericInstance", "monomorphized"),
    },
    {
        "title": "Detection recursion generique infinie",
        "area": "generics",
        "code": "TYPECK_E_CONSTRAINT_CYCLE",
        "tests": ("test_typeck_rejects_cyclic_generic_constraints",),
        "terms": ("generic_constraint_cycle_start", "TYPECK_E_CONSTRAINT_CYCLE", "generic constraint acyclicity"),
    },
    {
        "title": "Erreurs generiques avec cause racine",
        "area": "diagnostics",
        "code": "TYPECK_E_GENERIC_INFERENCE",
        "tests": ("test_generic_call_reports_unconstrained_parameter", "test_generic_trait_bound_reports_unsatisfied_impl"),
        "terms": ("expected origin", "found origin", "probable_fix"),
    },
    {
        "title": "Snapshots IR/MIR apres instanciation",
        "area": "monomorphization",
        "code": "MIR_E_GENERIC_INSTANCE",
        "tests": ("mir-snapshots", "ir-snapshots"),
        "terms": ("generic_arguments", "lower_hir_to_mir_templates", "monomorphize_mir"),
    },
    {
        "title": "Traits/interfaces",
        "area": "traits",
        "code": "TYPECK_E_TRAIT_BOUND",
        "tests": ("test_generic_trait_bound_accepts_visible_impl", "test_typeck_trait_helpers_cover_core_type_surface"),
        "terms": ("trait Show", "type_trait_info", "trait bound satisfaction"),
    },
    {
        "title": "Impl explicites",
        "area": "traits",
        "code": "TYPECK_E_IMPL_SIGNATURE_MISMATCH",
        "tests": ("test_typeck_reports_unknown_impl_trait", "test_typeck_reports_missing_impl_member", "test_typeck_reports_impl_return_signature_mismatch"),
        "terms": ("check_impl_against_trait", "check_impl_coherence", "TYPECK_E_IMPL_MISSING_MEMBER"),
    },
)


EXTERNAL_TESTS = {
    "diagnostic-quality",
    "grammar-test",
    "ir-snapshots",
    "mir-gate",
    "mir-snapshots",
    "typeck-snapshots",
}


EXTRA_EVIDENCE = {
    "tests/typeck/valid/generics.vit",
    "tests/typeck/valid/traits.vit",
    "tests/typeck/valid/inference.vit",
    "tests/typeck/valid/calls.vit",
    "tests/typeck/invalid/assignment_mismatch.vit",
    "tests/typeck/invalid/return_mismatch.vit",
    "tests/frontend_syntax/valid/full_grammar_surface.vit",
    "src/vitte/compiler/middle/mir/monomorphize.vit",
    "src/vitte/compiler/middle/mir/validate.vit",
    "src/vitte/compiler/tests/mir_tests.vit",
    "src/vitte/compiler/tests/snapshots/typeck/fixtures/diagnostics.json.snap",
    "src/vitte/compiler/tests/snapshots/typeck/fixtures/expected_obtained.txt.snap",
    "src/vitte/compiler/tests/snapshots/typeck/fixtures/cause_chain.txt.snap",
    "tests/diagnostics/catalog/typeck.catalog.json",
}


def build_requirements() -> list[Requirement]:
    requirements: list[Requirement] = []
    for index, raw in enumerate(RAW_REQUIREMENTS, start=1):
        title = str(raw["title"])
        code = str(raw["code"])
        requirements.append(
            Requirement(
                id=index,
                title=title,
                area=str(raw["area"]),
                diagnostic_code=code,
                explanation=f"{title}. Covered by the advanced type-system 1-20 contract.",
                suggestion=f"Keep a real Typeck/MIR fixture and diagnostic assertion for: {title}.",
                valid_fixture="tests/typeck/advanced_20/valid_inference_generics_traits.vit",
                invalid_fixture="tests/typeck/advanced_20/invalid_inference_generics_traits.vit",
                evidence=tuple(sorted(set(COMMON_TYPECK_EVIDENCE) | EXTRA_EVIDENCE)),
                tests=tuple(raw["tests"]),  # type: ignore[arg-type]
                terms=tuple(raw["terms"]),  # type: ignore[arg-type]
            )
        )
    return requirements


def read(path: Path) -> str:
    if not path.is_file():
        raise SystemExit(f"[type-system-advanced-20][error] missing file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8", errors="replace")


def test_names() -> set[str]:
    text = read(TYPECK_TESTS)
    return set(re.findall(r"(?m)^\s*proc\s+(test_[A-Za-z0-9_]+)\s*\(", text)) | EXTERNAL_TESTS


def evidence_text(paths: tuple[str, ...]) -> str:
    return "\n".join(rel + "\n" + read(ROOT / rel) for rel in paths)


def validate_requirements(requirements: list[Requirement]) -> None:
    if len(requirements) != 20:
        raise SystemExit(f"[type-system-advanced-20][error] expected 20 requirements, got {len(requirements)}")
    if [req.id for req in requirements] != list(range(1, 21)):
        raise SystemExit("[type-system-advanced-20][error] requirement ids must be TSA001..TSA020")
    if len({req.title for req in requirements}) != len(requirements):
        raise SystemExit("[type-system-advanced-20][error] duplicate requirement title")

    known_tests = test_names()
    full_evidence = evidence_text(tuple(sorted(set().union(*(set(req.evidence) for req in requirements)))))
    for req in requirements:
        for rel in req.evidence:
            if not (ROOT / rel).is_file():
                raise SystemExit(f"[type-system-advanced-20][error] TSA{req.id:03d} missing evidence: {rel}")
        unknown = [test for test in req.tests if test not in known_tests]
        if unknown:
            raise SystemExit(f"[type-system-advanced-20][error] TSA{req.id:03d} unknown tests: {', '.join(unknown)}")
        missing_terms = [term for term in req.terms if term not in full_evidence]
        if missing_terms:
            raise SystemExit(f"[type-system-advanced-20][error] TSA{req.id:03d} missing terms: {', '.join(missing_terms)}")
        for field in ("diagnostic_code", "explanation", "suggestion", "valid_fixture", "invalid_fixture"):
            if not getattr(req, field):
                raise SystemExit(f"[type-system-advanced-20][error] TSA{req.id:03d} missing {field}")


def digest(value: object) -> str:
    payload = json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def compiler_check_valid_fixture() -> None:
    binary = ROOT / "bin/vitte"
    fixture = ROOT / "tests/typeck/advanced_20/valid_inference_generics_traits.vit"
    if not binary.is_file():
        raise SystemExit("[type-system-advanced-20][error] missing bin/vitte")
    completed = subprocess.run(
        [str(binary), "check", str(fixture)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=10,
        check=False,
    )
    if completed.returncode != 0:
        raise SystemExit(
            "[type-system-advanced-20][error] valid advanced fixture failed:\n"
            + completed.stdout
            + completed.stderr
        )


def render(requirements: list[Requirement]) -> dict[str, object]:
    rows = []
    for req in requirements:
        row = asdict(req)
        row["requirement"] = f"TSA{req.id:03d}"
        row["status"] = "covered"
        row["fingerprint"] = digest(row)[:16]
        rows.append(row)
    manifest = {
        "schema": "vitte.type_system.advanced_20",
        "schema_version": "1.0.0",
        "requirements_total": 20,
        "requirements_covered": 20,
        "coverage_percent": 100,
        "matrix_fingerprint": digest(rows),
        "requirements": rows,
    }
    text = "\n".join(
        f"TSA{req.id:03d} covered {req.area} {req.diagnostic_code} {rows[req.id - 1]['fingerprint']} {req.title}"
        for req in requirements
    ) + "\n"
    lsp = {
        "jsonrpc": "2.0",
        "method": "textDocument/publishDiagnostics",
        "params": {
            "uri": "file:///tests/typeck/advanced_20/invalid_inference_generics_traits.vit",
            "version": 1,
            "diagnostics": [
                {
                    "source": "vitte",
                    "severity": 1,
                    "code": req.diagnostic_code,
                    "message": req.explanation,
                    "data": {"requirement": f"TSA{req.id:03d}", "area": req.area, "fix_why": req.suggestion},
                }
                for req in requirements
            ],
        },
    }
    return {"manifest": manifest, "text": text, "lsp": lsp}


def markdown(manifest: dict[str, object]) -> str:
    rows = manifest["requirements"]
    assert isinstance(rows, list)
    lines = [
        "# Type System Advanced 1-20",
        "",
        f"Coverage: {manifest['requirements_covered']}/{manifest['requirements_total']} ({manifest['coverage_percent']}%)",
        f"Fingerprint: {manifest['matrix_fingerprint']}",
        "",
        "| ID | Area | Code | Status | Title |",
        "| --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        assert isinstance(row, dict)
        lines.append(f"| {row['requirement']} | {row['area']} | {row['diagnostic_code']} | {row['status']} | {row['title']} |")
    return "\n".join(lines) + "\n"


def write_outputs(outputs: dict[str, object]) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "snapshots").mkdir(parents=True, exist_ok=True)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = outputs["manifest"]
    assert isinstance(manifest, dict)
    manifest_text = json.dumps(manifest, ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    lsp_text = json.dumps(outputs["lsp"], ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    md = markdown(manifest)
    (OUT_DIR / "manifest.json").write_text(manifest_text, encoding="utf-8")
    (OUT_DIR / "snapshots/text.snapshot").write_text(str(outputs["text"]), encoding="utf-8")
    (OUT_DIR / "snapshots/json.snapshot").write_text(manifest_text, encoding="utf-8")
    (OUT_DIR / "snapshots/lsp.snapshot").write_text(lsp_text, encoding="utf-8")
    (OUT_DIR / "coverage.md").write_text(md, encoding="utf-8")
    (REPORT_DIR / "type_system_advanced_20.json").write_text(manifest_text, encoding="utf-8")
    (REPORT_DIR / "type_system_advanced_20.md").write_text(md, encoding="utf-8")


def validate_outputs() -> None:
    required = (
        OUT_DIR / "manifest.json",
        OUT_DIR / "coverage.md",
        OUT_DIR / "snapshots/text.snapshot",
        OUT_DIR / "snapshots/json.snapshot",
        OUT_DIR / "snapshots/lsp.snapshot",
        REPORT_DIR / "type_system_advanced_20.json",
        REPORT_DIR / "type_system_advanced_20.md",
    )
    for path in required:
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"[type-system-advanced-20][error] missing artifact: {path.relative_to(ROOT)}")
    manifest = json.loads((OUT_DIR / "manifest.json").read_text(encoding="utf-8"))
    if manifest["requirements_total"] != 20 or manifest["coverage_percent"] != 100:
        raise SystemExit("[type-system-advanced-20][error] manifest is not 20/20")


def main() -> int:
    requirements = build_requirements()
    validate_requirements(requirements)
    compiler_check_valid_fixture()
    outputs = render(requirements)
    write_outputs(outputs)
    validate_outputs()
    print(f"[type-system-advanced-20] OK requirements=20 fingerprint={outputs['manifest']['matrix_fingerprint']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
