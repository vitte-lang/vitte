#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile

from typeck_differential_test import DEFAULT_BINS, diagnostic_projection, resolve_binaries


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "typeck_fuzz.json"
SEED = 0x5649545445
LITERALS = {
    "i32": ("0", "1", "-7", "42"),
    "string": ('""', '"text"', '"with space"'),
    "bool": ("true", "false"),
}


def assignment_source(index: int, rng: random.Random) -> str:
    declared = rng.choice(sorted(LITERALS))
    actual = rng.choice(sorted(LITERALS))
    initial = rng.choice(LITERALS[declared])
    assigned = rng.choice(LITERALS[actual])
    name = rng.choice(("x", "value", "typed_value", f"item_{index}"))
    semicolon = rng.choice(("", ";"))
    blank = "\n" * rng.randrange(0, 3)
    return (
        f"space tests/typeck/fuzz/assignment_{index}\n{blank}"
        "proc main() -> int {\n"
        f"  let {name}: {declared} = {initial}{semicolon}\n"
        f"  set {name} = {assigned}{semicolon}\n"
        "  give 0\n"
        "}\n"
    )


def condition_source(index: int, rng: random.Random) -> str:
    condition_type = rng.choice(sorted(LITERALS))
    condition = rng.choice(LITERALS[condition_type])
    return (
        f"space tests/typeck/fuzz/condition_{index}\n\n"
        "proc main() -> int {\n"
        f"  if {condition} {{\n"
        "    give 1\n"
        "  }\n"
        "  give 0\n"
        "}\n"
    )


def binary_source(index: int, rng: random.Random) -> str:
    left_type = rng.choice(sorted(LITERALS))
    right_type = rng.choice(sorted(LITERALS))
    left = rng.choice(LITERALS[left_type])
    right = rng.choice(LITERALS[right_type])
    operator = rng.choice(("+", "-", "*", "=="))
    return (
        f"space tests/typeck/fuzz/binary_{index}\n\n"
        "proc main() -> int {\n"
        f"  let result = {left} {operator} {right}\n"
        "  give 0\n"
        "}\n"
    )


def return_source(index: int, rng: random.Random) -> str:
    declared = rng.choice(sorted(LITERALS))
    actual = rng.choice(sorted(LITERALS))
    value = rng.choice(LITERALS[actual])
    return (
        f"space tests/typeck/fuzz/return_{index}\n\n"
        f"proc value() -> {declared} {{ give {value}; }}\n"
        "proc main() -> int { give 0; }\n"
    )


def call_source(index: int, rng: random.Random) -> str:
    parameter_type = rng.choice(sorted(LITERALS))
    actual_type = rng.choice(sorted(LITERALS))
    value = rng.choice(LITERALS[actual_type])
    arity_suffix = rng.choice(("", ", 0"))
    return (
        f"space tests/typeck/fuzz/call_{index}\n\n"
        f"proc take(value: {parameter_type}) -> int {{ give 0; }}\n"
        f"proc main() -> int {{ give take({value}{arity_suffix}); }}\n"
    )


def generic_source(index: int, rng: random.Random) -> str:
    actual_type = rng.choice(sorted(LITERALS))
    value = rng.choice(LITERALS[actual_type])
    explicit = rng.choice(("", f"[{actual_type}]"))
    return (
        f"space tests/typeck/fuzz/generic_{index}\n\n"
        "proc identity[T](value: T) -> T { give value; }\n"
        f"proc main() -> int {{ let value = identity{explicit}({value}); give 0; }}\n"
    )


def malformed_source(index: int, rng: random.Random) -> str:
    base = assignment_source(index, rng)
    mutation = rng.randrange(5)
    if mutation == 0:
        return base[: rng.randrange(1, len(base))]
    if mutation == 1:
        return base.replace("}", "", 1)
    if mutation == 2:
        return base.replace("=", "= @", 1)
    if mutation == 3:
        return base.replace("proc main()", "proc main(", 1)
    return base + 'let unterminated = "\n'


GENERATORS = (
    ("assignment", assignment_source),
    ("condition", condition_source),
    ("binary", binary_source),
    ("return", return_source),
    ("call", call_source),
    ("generic", generic_source),
    ("malformed", malformed_source),
)


def generate_corpus(count: int) -> list[tuple[str, str, str]]:
    rng = random.Random(SEED)
    corpus: list[tuple[str, str, str]] = []
    seen: set[str] = set()
    attempts = 0
    while len(corpus) < count and attempts < count * 20:
        attempts += 1
        category, generator = GENERATORS[len(corpus) % len(GENERATORS)]
        source = generator(attempts, rng)
        digest = hashlib.sha256(source.encode("utf-8")).hexdigest()
        if digest in seen:
            continue
        seen.add(digest)
        corpus.append((f"case_{len(corpus):04d}", category, source))
    if len(corpus) != count:
        raise AssertionError(f"could only generate {len(corpus)} unique cases out of {count}")
    return corpus


def parse_payload(stdout: str, binary: Path, case_name: str) -> dict[str, object]:
    for line in reversed([line.strip() for line in stdout.splitlines() if line.strip()]):
        try:
            payload = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(payload, dict) and payload.get("schema") == "vitte.compiler.surface":
            return payload
    raise AssertionError(f"{binary.name}/{case_name}: missing compiler surface JSON")


def run(binary: Path, source_path: Path, case_name: str) -> tuple[int, dict[str, object]]:
    completed = subprocess.run(
        [str(binary), "check", "--diagnostics-json", "--lang=en", str(source_path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=10,
        check=False,
    )
    if completed.returncode < 0:
        raise AssertionError(f"{binary.name}/{case_name}: compiler terminated by signal {-completed.returncode}")
    if completed.returncode not in (0, 1):
        raise AssertionError(f"{binary.name}/{case_name}: unexpected exit {completed.returncode}")
    payload = parse_payload(completed.stdout, binary, case_name)
    if payload.get("surface") != "diagnostics":
        raise AssertionError(f"{binary.name}/{case_name}: wrong JSON surface")
    if (completed.returncode == 0) != (payload.get("valid") is True):
        raise AssertionError(f"{binary.name}/{case_name}: exit status and valid flag disagree")
    report = payload.get("primary_report")
    diagnostics = report.get("diagnostics", []) if isinstance(report, dict) else []
    for diagnostic in diagnostics:
        if not isinstance(diagnostic, dict):
            raise AssertionError(f"{binary.name}/{case_name}: non-object diagnostic")
        code = str(diagnostic.get("code", ""))
        severity = diagnostic.get("severity")
        if "INTERNAL" in code or "PANIC" in code or severity == "fatal":
            raise AssertionError(f"{binary.name}/{case_name}: compiler failure diagnostic {code}")
    return completed.returncode, payload


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cases", type=int, default=int(os.environ.get("TYPECK_FUZZ_CASES", "42")))
    parser.add_argument("--bins", help=f"compiler paths separated by {os.pathsep!r}")
    args = parser.parse_args()
    if args.cases < len(GENERATORS) or args.cases > 1000:
        raise AssertionError("--cases must be between 7 and 1000")
    binaries = resolve_binaries(args.bins or os.environ.get("TYPECK_FUZZ_BINS"))
    corpus = generate_corpus(args.cases)
    categories = {name: 0 for name, _ in GENERATORS}
    comparisons = 0
    deterministic_replays = 0

    with tempfile.TemporaryDirectory(prefix="vitte-typeck-fuzz-") as tmp:
        tmp_dir = Path(tmp)
        for index, (case_name, category, source) in enumerate(corpus):
            categories[category] += 1
            source_path = tmp_dir / f"{case_name}.vit"
            source_path.write_text(source, encoding="utf-8")
            projections: list[dict[str, object]] = []
            for binary in binaries:
                _, payload = run(binary, source_path, case_name)
                projections.append(diagnostic_projection(payload))
            reference = projections[0]
            for binary, projection in zip(binaries[1:], projections[1:]):
                comparisons += 1
                if projection != reference:
                    raise AssertionError(
                        f"{case_name}/{category}: {binaries[0].name} and {binary.name} disagree"
                    )
            if index < 8:
                _, replay = run(binaries[0], source_path, case_name)
                deterministic_replays += 1
                if diagnostic_projection(replay) != reference:
                    raise AssertionError(f"{case_name}/{category}: non-deterministic result")

    compiler_runs = len(corpus) * len(binaries) + deterministic_replays
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps({
        "schema": "vitte.typeck-fuzz",
        "schema_version": "1.0.0",
        "status": "pass",
        "seed": SEED,
        "case_count": len(corpus),
        "compiler_runs": compiler_runs,
        "differential_comparisons": comparisons,
        "deterministic_replays": deterministic_replays,
        "categories": categories,
        "binaries": [str(binary.relative_to(ROOT)) for binary in binaries],
    }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"[typeck-fuzz] pass seed={SEED} bins={len(binaries)} cases={len(corpus)} "
        f"runs={compiler_runs} comparisons={comparisons}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, subprocess.TimeoutExpired) as error:
        print(f"[typeck-fuzz][error] {error}", file=sys.stderr)
        raise SystemExit(1)
