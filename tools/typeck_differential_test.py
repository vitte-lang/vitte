#!/usr/bin/env python3
from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BINS = [ROOT / "bin" / "vittec0", ROOT / "bin" / "vittec1", ROOT / "bin" / "vittec"]
REPORT = ROOT / "target" / "reports" / "typeck_differential.json"


@dataclass(frozen=True)
class Case:
    name: str
    source: str
    valid: bool
    required_code: str = ""
    required_rule: str = ""


def source_for_assignment(name: str, declared_type: str, initial: str, assigned: str, padding: str = "") -> str:
    return (
        f"space tests/typeck/differential/{name}\n\n"
        f"proc main() -> int {{{padding}\n"
        f"  let {name}: {declared_type} = {initial}\n"
        f"  set {name} = {assigned}\n"
        "  give 0\n"
        "}\n"
    )


def generated_cases() -> list[Case]:
    primitives = {
        "i32": "1",
        "string": '"text"',
        "bool": "true",
    }
    cases: list[Case] = []
    for declared_type, initial in primitives.items():
        cases.append(Case(
            f"identity_{declared_type}",
            source_for_assignment(f"value_{declared_type}", declared_type, initial, initial),
            True,
        ))
        for actual_type, assigned in primitives.items():
            if actual_type == declared_type:
                continue
            cases.append(Case(
                f"mismatch_{declared_type}_{actual_type}",
                source_for_assignment(f"value_{declared_type}_{actual_type}", declared_type, initial, assigned),
                False,
                "TYPECK_E_ASSIGN_MISMATCH",
            ))

    for index, name in enumerate(("x", "value", "renamed_value", "long_binding_name", "n0", "alpha")):
        padding = "\n" * (index % 3)
        cases.append(Case(
            f"alpha_spacing_{index}",
            source_for_assignment(name, "i32", "1", '"wrong"', padding),
            False,
            "TYPECK_E_ASSIGN_MISMATCH",
        ))

    for index, name in enumerate(("condition", "predicate", "renamed_condition")):
        cases.append(Case(
            f"invalid_condition_{index}",
            (
                f"space tests/typeck/differential/{name}\n\n"
                "proc main() -> int {\n"
                f"  if \"not-{name}\" {{ give 1 }}\n"
                "  give 0\n"
                "}\n"
            ),
            False,
            "TYPECK_E_CONDITION_TYPE",
        ))

    for operator_name, operator in (("equal", "=="), ("not_equal", "!=")):
        cases.append(Case(
            f"string_{operator_name}_condition",
            (
                f"space tests/typeck/differential/string_{operator_name}_condition\n\n"
                "proc main() -> int {\n"
                '  let condition: string = "expected"\n'
                f'  if condition {operator} "expected" {{ give 1 }}\n'
                "  give 0\n"
                "}\n"
            ),
            True,
        ))

    return_literals = {
        "i32": "1",
        "string": '"text"',
        "bool": "true",
    }
    for declared_type, valid_literal in return_literals.items():
        cases.append(Case(
            f"return_identity_{declared_type}",
            (
                f"space tests/typeck/differential/return_identity_{declared_type}\n\n"
                f"proc value() -> {declared_type} {{\n"
                f"  give {valid_literal}\n"
                "}\n"
                "proc main() -> int { give 0 }\n"
            ),
            True,
        ))
        mismatched_type = next(name for name in return_literals if name != declared_type)
        cases.append(Case(
            f"return_mismatch_{declared_type}_{mismatched_type}",
            (
                f"space tests/typeck/differential/return_mismatch_{declared_type}_{mismatched_type}\n\n"
                f"proc value() -> {declared_type} {{\n"
                f"  give {return_literals[mismatched_type]}\n"
                "}\n"
                "proc main() -> int { give 0 }\n"
            ),
            False,
            "TYPECK_E_RETURN_MISMATCH",
            "procedure return compatibility",
        ))

    for parameter_type, valid_literal in return_literals.items():
        cases.append(Case(
            f"argument_identity_{parameter_type}",
            (
                f"space tests/typeck/differential/argument_identity_{parameter_type}\n\n"
                f"proc take(value: {parameter_type}) -> int {{\n"
                "  give 0\n"
                "}\n"
                "proc main() -> int {\n"
                f"  give take({valid_literal})\n"
                "}\n"
            ),
            True,
        ))
        mismatched_type = next(name for name in return_literals if name != parameter_type)
        cases.append(Case(
            f"argument_mismatch_{parameter_type}_{mismatched_type}",
            (
                f"space tests/typeck/differential/argument_mismatch_{parameter_type}_{mismatched_type}\n\n"
                f"proc take(value: {parameter_type}) -> int {{\n"
                "  give 0\n"
                "}\n"
                "proc main() -> int {\n"
                f"  give take({return_literals[mismatched_type]})\n"
                "}\n"
            ),
            False,
            "TYPECK_E_ARGUMENT_MISMATCH",
            "function argument compatibility",
        ))

    for suffix, arguments in (("too_few", ""), ("too_many", "1, 2")):
        cases.append(Case(
            f"call_arity_{suffix}",
            (
                f"space tests/typeck/differential/call_arity_{suffix}\n\n"
                "proc take(value: int) -> int {\n"
                "  give value\n"
                "}\n"
                "proc main() -> int {\n"
                f"  give take({arguments})\n"
                "}\n"
            ),
            False,
            "TYPECK_E_CALL_ARITY",
            "function call arity",
        ))
    cases.append(Case(
        "call_arity_nested_arguments",
        (
            "space tests/typeck/differential/call_arity_nested_arguments\n\n"
            "proc accept(value: bool) -> int {\n"
            "  give 0\n"
            "}\n"
            "proc main() -> int {\n"
            "  give accept(find(\"x\", \"x\") >= 0 or find(\"x\", \"y\") >= 0)\n"
            "}\n"
        ),
        True,
    ))
    cases.append(Case(
        "generic_inference_from_argument",
        (
            "space tests/typeck/differential/generic_inference_from_argument\n\n"
            "proc identity[T](value: T) -> T { give value }\n"
            "proc main() -> int {\n"
            "  give identity(7)\n"
            "}\n"
        ),
        True,
    ))
    cases.append(Case(
        "generic_inference_explicit",
        (
            "space tests/typeck/differential/generic_inference_explicit\n\n"
            "proc make[T]() -> T { give 0 as T }\n"
            "proc main() -> int {\n"
            "  give make<int>()\n"
            "}\n"
        ),
        True,
    ))
    cases.append(Case(
        "generic_inference_unconstrained",
        (
            "space tests/typeck/differential/generic_inference_unconstrained\n\n"
            "proc make[T]() -> T { give 0 as T }\n"
            "proc main() -> int {\n"
            "  give make()\n"
            "}\n"
        ),
        False,
        "TYPECK_E_GENERIC_INFERENCE",
        "generic inference completeness",
    ))
    cases.append(Case(
        "trait_bound_satisfied",
        (
            "space tests/typeck/differential/trait_bound_satisfied\n\n"
            "trait Show { proc show(self: Self) -> string; }\n"
            "impl Show for int { proc show(self: Self) -> string { give \"int\" } }\n"
            "proc keep[T: Show](value: T) -> T { give value }\n"
            "proc main() -> int {\n"
            "  give keep(7)\n"
            "}\n"
        ),
        True,
    ))
    cases.append(Case(
        "trait_bound_unsatisfied",
        (
            "space tests/typeck/differential/trait_bound_unsatisfied\n\n"
            "trait Show { proc show(self: Self) -> string; }\n"
            "proc keep[T: Show](value: T) -> T { give value }\n"
            "proc main() -> int {\n"
            "  give keep(7)\n"
            "}\n"
        ),
        False,
        "TYPECK_E_TRAIT_BOUND",
        "trait bound satisfaction",
    ))
    cases.append(Case(
        "trait_impls_disjoint",
        (
            "space tests/typeck/differential/trait_impls_disjoint\n\n"
            "trait Show { proc show(self: Self) -> string; }\n"
            "impl Show for int { proc show(self: Self) -> string { give \"int\" } }\n"
            "impl Show for string { proc show(self: Self) -> string { give self } }\n"
            "proc main() -> int { give 0 }\n"
        ),
        True,
    ))
    cases.append(Case(
        "trait_impls_conflict",
        (
            "space tests/typeck/differential/trait_impls_conflict\n\n"
            "trait Show { proc show(self: Self) -> string; }\n"
            "impl Show for int { proc show(self: Self) -> string { give \"first\" } }\n"
            "impl Show for int { proc show(self: Self) -> string { give \"second\" } }\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_CONFLICTING_IMPL",
        "trait implementation coherence",
    ))
    cases.append(Case(
        "generic_constraints_acyclic",
        (
            "space tests/typeck/differential/generic_constraints_acyclic\n\n"
            "trait Show { proc show(self: Self) -> string; }\n"
            "proc keep[T: Show, U: T](value: U) -> U { give value }\n"
            "proc main() -> int { give 0 }\n"
        ),
        True,
    ))
    cases.append(Case(
        "generic_constraints_cycle",
        (
            "space tests/typeck/differential/generic_constraints_cycle\n\n"
            "proc cycle[T: U, U: T](value: T) -> T { give value }\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_CONSTRAINT_CYCLE",
        "generic constraint acyclicity",
    ))
    cases.append(Case(
        "numeric_cast_explicit",
        (
            "space tests/typeck/differential/numeric_cast_explicit\n\n"
            "proc main() -> int {\n"
            "  let value: i8 = 7 as i8\n"
            "  give value as int\n"
            "}\n"
        ),
        True,
    ))
    cases.append(Case(
        "invalid_scalar_cast",
        (
            "space tests/typeck/differential/invalid_scalar_cast\n\n"
            "proc main() -> int {\n"
            "  give \"oops\" as int\n"
            "}\n"
        ),
        False,
        "TYPECK_E_INVALID_CAST",
        "explicit cast validity",
    ))
    cases.append(Case(
        "numeric_widening_implicit",
        (
            "space tests/typeck/differential/numeric_widening_implicit\n\n"
            "proc main() -> int {\n"
            "  let small: u8 = 1\n"
            "  let wide: i64 = small\n"
            "  give 0\n"
            "}\n"
        ),
        True,
    ))
    cases.append(Case(
        "numeric_narrowing_requires_cast",
        (
            "space tests/typeck/differential/numeric_narrowing_requires_cast\n\n"
            "proc main() -> int {\n"
            "  let wide: i64 = 1\n"
            "  let narrow: i8 = wide\n"
            "  give 0\n"
            "}\n"
        ),
        False,
        "TYPECK_E_ASSIGN_MISMATCH",
        "assignment type compatibility",
    ))
    cases.append(Case(
        "match_bool_exhaustive",
        (
            "space tests/typeck/differential/match_bool_exhaustive\n\n"
            "proc classify(value: bool) -> int {\n"
            "  match value {\n"
            "    case true => 1;\n"
            "    case false => 0;\n"
            "  }\n"
            "  give 0\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        True,
    ))
    cases.append(Case(
        "match_bool_missing_false",
        (
            "space tests/typeck/differential/match_bool_missing_false\n\n"
            "proc classify(value: bool) -> int {\n"
            "  match value {\n"
            "    case true => 1;\n"
            "  }\n"
            "  give 0\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_MATCH_NON_EXHAUSTIVE",
        "pattern coverage completeness",
    ))
    cases.append(Case(
        "match_bool_fallback",
        (
            "space tests/typeck/differential/match_bool_fallback\n\n"
            "proc classify(value: bool) -> int {\n"
            "  match value {\n"
            "    case true => 1;\n"
            "    case _ => 0;\n"
            "  }\n"
            "  give 0\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        True,
    ))
    cases.append(Case(
        "match_pick_exhaustive",
        (
            "space tests/typeck/differential/match_pick_exhaustive\n\n"
            "pick Choice {\n"
            "  Left,\n"
            "  Right,\n"
            "}\n"
            "proc classify(value: Choice) -> int {\n"
            "  match value {\n"
            "    case Choice.Left => 1;\n"
            "    case Choice.Right => 2;\n"
            "  }\n"
            "  give 0\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        True,
    ))
    cases.append(Case(
        "match_pick_missing_variant",
        (
            "space tests/typeck/differential/match_pick_missing_variant\n\n"
            "pick Choice { case Left, case Right }\n"
            "proc classify(value: Choice) -> int {\n"
            "  match value {\n"
            "    case Choice::Left => 1;\n"
            "  }\n"
            "  give 0\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_MATCH_NON_EXHAUSTIVE",
        "pattern coverage completeness",
    ))
    cases.append(Case(
        "impl_signature_compatible",
        (
            "space tests/typeck/differential/impl_signature_compatible\n\n"
            "form Point { x: int }\n"
            "trait Show {\n"
            "  proc show(self: Self, level: int) -> string;\n"
            "}\n"
            "impl Show for Point {\n"
            "  proc show(self: Self, level: int) -> string { give \"point\" }\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        True,
    ))
    cases.append(Case(
        "impl_signature_return_mismatch",
        (
            "space tests/typeck/differential/impl_signature_return_mismatch\n\n"
            "form Point { x: int }\n"
            "trait Show { proc show(self: Self) -> string; }\n"
            "impl Show for Point { proc show(self: Self) -> int { give 0 } }\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_IMPL_SIGNATURE_MISMATCH",
        "implementation signature compatibility",
    ))
    cases.append(Case(
        "impl_signature_parameter_mismatch",
        (
            "space tests/typeck/differential/impl_signature_parameter_mismatch\n\n"
            "form Point { x: int }\n"
            "trait Show { proc show(self: Self, level: int) -> string; }\n"
            "impl Show for Point { proc show(self: Self, level: bool) -> string { give \"point\" } }\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_IMPL_SIGNATURE_MISMATCH",
        "implementation signature compatibility",
    ))
    cases.append(Case(
        "impl_signature_arity_mismatch",
        (
            "space tests/typeck/differential/impl_signature_arity_mismatch\n\n"
            "form Point { x: int }\n"
            "trait Show { proc show(self: Self, level: int) -> string; }\n"
            "impl Show for Point { proc show(self: Self) -> string { give \"point\" } }\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_IMPL_SIGNATURE_MISMATCH",
        "implementation signature compatibility",
    ))
    cases.append(Case(
        "impl_members_complete",
        (
            "space tests/typeck/differential/impl_members_complete\n\n"
            "form Point { x: int }\n"
            "trait Display {\n"
            "  proc show(self: Self) -> string;\n"
            "  proc debug(self: Self) -> string;\n"
            "}\n"
            "impl Display for Point {\n"
            "  proc debug(self: Self) -> string { give \"debug\" }\n"
            "  proc show(self: Self) -> string { give \"point\" }\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        True,
    ))
    cases.append(Case(
        "impl_member_missing",
        (
            "space tests/typeck/differential/impl_member_missing\n\n"
            "form Point { x: int }\n"
            "trait Display {\n"
            "  proc show(self: Self) -> string;\n"
            "  proc debug(self: Self) -> string;\n"
            "}\n"
            "impl Display for Point {\n"
            "  proc show(self: Self) -> string { give \"point\" }\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_IMPL_MISSING_MEMBER",
        "implementation completeness",
    ))
    cases.append(Case(
        "impl_trait_declared_later",
        (
            "space tests/typeck/differential/impl_trait_declared_later\n\n"
            "form Point { x: int }\n"
            "impl Show for Point { proc show(self: Self) -> string { give \"point\" } }\n"
            "trait Show { proc show(self: Self) -> string; }\n"
            "proc main() -> int { give 0 }\n"
        ),
        True,
    ))
    cases.append(Case(
        "impl_trait_unknown",
        (
            "space tests/typeck/differential/impl_trait_unknown\n\n"
            "form Point { x: int }\n"
            "impl Missing for Point {\n"
            "  proc show(self: Self) -> string { give \"point\" }\n"
            "}\n"
            "proc main() -> int { give 0 }\n"
        ),
        False,
        "TYPECK_E_IMPL_UNKNOWN_TRAIT",
        "implementation trait resolution",
    ))
    cases.append(Case(
        "method_provider_unique",
        (
            "space tests/typeck/differential/method_provider_unique\n\n"
            "form Point { x: int }\n"
            "trait Show { proc render(self: Self) -> string; }\n"
            "impl Show for Point { proc render(self: Self) -> string { give \"point\" } }\n"
            "proc main() -> int {\n"
            "  let point = Point { x: 1 }\n"
            "  let text = point.render()\n"
            "  give 0\n"
            "}\n"
        ),
        True,
    ))
    cases.append(Case(
        "method_provider_ambiguous",
        (
            "space tests/typeck/differential/method_provider_ambiguous\n\n"
            "form Point { x: int }\n"
            "trait Show { proc render(self: Self) -> string; }\n"
            "trait Debug { proc render(self: Self) -> string; }\n"
            "impl Show for Point { proc render(self: Self) -> string { give \"show\" } }\n"
            "impl Debug for Point { proc render(self: Self) -> string { give \"debug\" } }\n"
            "proc main() -> int {\n"
            "  let point = Point { x: 1 }\n"
            "  let text = point.render()\n"
            "  give 0\n"
            "}\n"
        ),
        False,
        "TYPECK_E_AMBIGUOUS_METHOD",
        "unique method candidate",
    ))
    cases.append(Case(
        "bound_method_arity_valid",
        (
            "space tests/typeck/differential/bound_method_arity_valid\n\n"
            "form Point { x: int }\n"
            "trait Show { proc show(self: Self, level: int) -> string; }\n"
            "impl Show for Point { proc show(self: Self, level: int) -> string { give \"point\" } }\n"
            "proc main() -> int {\n"
            "  let point = Point { x: 1 }\n"
            "  let text = point.show(7)\n"
            "  give 0\n"
            "}\n"
        ),
        True,
    ))
    cases.append(Case(
        "bound_method_arity_missing_argument",
        (
            "space tests/typeck/differential/bound_method_arity_missing_argument\n\n"
            "form Point { x: int }\n"
            "trait Show { proc show(self: Self, level: int) -> string; }\n"
            "impl Show for Point { proc show(self: Self, level: int) -> string { give \"point\" } }\n"
            "proc main() -> int {\n"
            "  let point = Point { x: 1 }\n"
            "  let text = point.show()\n"
            "  give 0\n"
            "}\n"
        ),
        False,
        "TYPECK_E_CALL_ARITY",
        "function call arity",
    ))
    return cases


def parse_json_output(stdout: str, binary: Path, case: Case) -> dict[str, object]:
    lines = [line.strip() for line in stdout.splitlines() if line.strip()]
    for line in reversed(lines):
        try:
            payload = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(payload, dict) and payload.get("surface") == "diagnostics":
            return payload
    raise AssertionError(f"{binary.name}/{case.name}: no diagnostics JSON object in stdout")


def diagnostic_projection(payload: dict[str, object]) -> dict[str, object]:
    report = payload.get("primary_report")
    diagnostics = report.get("diagnostics", []) if isinstance(report, dict) else []
    projected = []
    for diagnostic in diagnostics:
        if not isinstance(diagnostic, dict):
            continue
        projected.append({
            "code": diagnostic.get("code"),
            "severity": diagnostic.get("severity"),
            "phase": diagnostic.get("phase"),
            "rule": diagnostic.get("rule"),
            "root_cause": diagnostic.get("root_cause"),
            "cause_chain": diagnostic.get("cause_chain"),
            "subject_type": diagnostic.get("subject_type"),
        })
    return {
        "valid": payload.get("valid"),
        "typeck_ok": payload.get("typeck_ok"),
        "pipeline_failed_at": payload.get("pipeline_failed_at"),
        "error_count": report.get("error_count") if isinstance(report, dict) else None,
        "diagnostics": projected,
    }


def run_case(binary: Path, source_path: Path, case: Case) -> tuple[int, dict[str, object]]:
    completed = subprocess.run(
        [str(binary), "check", "--diagnostics-json", "--lang=en", str(source_path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=15,
        check=False,
    )
    if completed.returncode < 0:
        raise AssertionError(f"{binary.name}/{case.name}: terminated by signal {-completed.returncode}")
    if completed.returncode not in (0, 1):
        raise AssertionError(
            f"{binary.name}/{case.name}: unexpected exit {completed.returncode}: {completed.stderr.strip()}"
        )
    return completed.returncode, parse_json_output(completed.stdout, binary, case)


def assert_oracle(binary: Path, case: Case, exit_code: int, payload: dict[str, object]) -> None:
    projection = diagnostic_projection(payload)
    diagnostics = projection["diagnostics"]
    if case.valid:
        if exit_code != 0 or projection["valid"] is not True or projection["typeck_ok"] is not True:
            raise AssertionError(f"{binary.name}/{case.name}: valid case rejected: {projection}")
        if diagnostics:
            raise AssertionError(f"{binary.name}/{case.name}: valid case emitted diagnostics: {diagnostics}")
        return

    if exit_code != 1 or projection["valid"] is not False or projection["typeck_ok"] is not False:
        raise AssertionError(f"{binary.name}/{case.name}: invalid case accepted: {projection}")
    if projection["pipeline_failed_at"] != "typeck":
        raise AssertionError(f"{binary.name}/{case.name}: failure did not originate in typeck")
    matching = [diag for diag in diagnostics if diag.get("code") == case.required_code]
    if not matching:
        raise AssertionError(f"{binary.name}/{case.name}: missing {case.required_code}: {diagnostics}")
    for diagnostic in matching:
        if diagnostic.get("phase") != "typeck" or diagnostic.get("severity") != "error":
            raise AssertionError(f"{binary.name}/{case.name}: malformed diagnostic contract: {diagnostic}")
        if case.required_rule and diagnostic.get("rule") != case.required_rule:
            raise AssertionError(
                f"{binary.name}/{case.name}: expected rule {case.required_rule!r}: {diagnostic}"
            )
        causes = diagnostic.get("cause_chain")
        if not isinstance(causes, list) or len(causes) < 2 or any(not cause for cause in causes):
            raise AssertionError(f"{binary.name}/{case.name}: diagnostic has no complete cause chain")


def resolve_binaries(raw: str | None) -> list[Path]:
    if raw:
        bins = [Path(value).resolve() for value in raw.split(os.pathsep) if value]
    else:
        bins = DEFAULT_BINS
    missing = [str(binary) for binary in bins if not binary.is_file() or not os.access(binary, os.X_OK)]
    if missing:
        raise AssertionError(f"missing executable compiler binaries: {', '.join(missing)}")
    if len(bins) < 2:
        raise AssertionError("differential testing requires at least two compiler binaries")
    return bins


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bins", help=f"compiler paths separated by {os.pathsep!r}")
    args = parser.parse_args()
    binaries = resolve_binaries(args.bins or os.environ.get("TYPECK_DIFF_BINS"))
    cases = generated_cases()
    comparisons = 0
    deterministic_replays = 0

    with tempfile.TemporaryDirectory(prefix="vitte-typeck-diff-") as tmp:
        tmp_dir = Path(tmp)
        for case_index, case in enumerate(cases):
            source_path = tmp_dir / f"{case.name}.vit"
            source_path.write_text(case.source, encoding="utf-8")
            projections: list[dict[str, object]] = []
            for binary in binaries:
                exit_code, payload = run_case(binary, source_path, case)
                assert_oracle(binary, case, exit_code, payload)
                projections.append(diagnostic_projection(payload))
            reference = projections[0]
            for binary, projection in zip(binaries[1:], projections[1:]):
                comparisons += 1
                if projection != reference:
                    raise AssertionError(
                        f"{case.name}: {binaries[0].name} and {binary.name} disagree:\n"
                        f"reference={reference}\nactual={projection}"
                    )

            if case_index < 6:
                replay_exit, replay_payload = run_case(binaries[0], source_path, case)
                assert_oracle(binaries[0], case, replay_exit, replay_payload)
                deterministic_replays += 1
                if diagnostic_projection(replay_payload) != reference:
                    raise AssertionError(f"{case.name}: repeated type checking is not deterministic")

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps({
        "schema": "vitte.typeck-differential-test",
        "schema_version": "1.0.0",
        "status": "pass",
        "binaries": [str(binary.relative_to(ROOT)) for binary in binaries],
        "case_count": len(cases),
        "compiler_runs": len(cases) * len(binaries) + deterministic_replays,
        "differential_comparisons": comparisons,
        "deterministic_replays": deterministic_replays,
        "properties": [
            "primitive identity assignments are accepted",
            "distinct primitive assignments are rejected",
            "alpha-renaming and whitespace preserve diagnostic codes",
            "non-truthy string conditions are rejected",
            "string equality and inequality produce boolean conditions",
            "primitive return contracts accept matching values and reject mismatches",
            "primitive argument contracts accept matching values and reject mismatches",
            "local calls reject missing and excess arguments",
            "generic calls accept constrained or explicit parameters and reject unconstrained parameters",
            "locally resolved generic types satisfy declared trait bounds",
            "duplicate local trait implementations violate coherence",
            "generic parameter constraints form an acyclic dependency graph",
            "explicit literal casts stay within supported scalar representations",
            "implicit numeric coercions are directional and lossless",
            "finite bool and local pick matches require complete case coverage or a fallback",
            "local trait implementations preserve method arity, parameter types, and return types",
            "local trait implementations provide every required method regardless of declaration order",
            "local impl trait names resolve after complete source-order-independent declaration indexing",
            "method dispatch requires one visible impl provider for the resolved receiver type",
            "bound method arity excludes the receiver Self parameter from explicit arguments",
            "every rejected type-system case carries a two-step cause chain",
            "stage binaries agree on normalized typeck results",
            "repeated checks are deterministic",
            "user programs do not terminate the compiler by signal",
        ],
    }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"[typeck-differential] pass bins={len(binaries)} cases={len(cases)} "
        f"runs={len(cases) * len(binaries) + deterministic_replays} comparisons={comparisons}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, subprocess.TimeoutExpired) as error:
        print(f"[typeck-differential][error] {error}", file=sys.stderr)
        raise SystemExit(1)
