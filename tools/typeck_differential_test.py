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
