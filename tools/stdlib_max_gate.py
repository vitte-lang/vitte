#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MATRIX_PATH = ROOT / "src" / "vitte" / "stdlib" / "stdlib_max_matrix.json"
MODULES_PATH = ROOT / "src" / "vitte" / "stdlib" / "stdlib_modules.json"
CONTRACT_TEST = ROOT / "src" / "vitte" / "stdlib" / "tests" / "std_max_surface_contracts.vit"
DOC_PATH = ROOT / "docs" / "compiler" / "stdlib_max_surface.md"
MAKEFILE_PATH = ROOT / "Makefile"
REPORT_DIR = ROOT / "target" / "reports"
JSON_REPORT = REPORT_DIR / "stdlib_max_gate.json"
MD_REPORT = REPORT_DIR / "stdlib_max_gate.md"


@dataclass(frozen=True)
class Failure:
    code: str
    detail: str


def load_json(path: Path) -> object:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def run_vitte_check(path: Path) -> tuple[bool, str]:
    proc = subprocess.run(
        [str(ROOT / "bin" / "vitte"), "check", str(path.relative_to(ROOT))],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return proc.returncode == 0, proc.stdout.strip()


def require_fragment(source: str, fragment: str) -> bool:
    return fragment in source


def proc_implementation_counts(source: str) -> tuple[int, int]:
    proc_count = 0
    native_count = 0
    lines = source.splitlines()
    index = 0
    while index < len(lines):
        line = lines[index]
        if not line.lstrip().startswith("proc "):
            index += 1
            continue

        proc_count += 1
        body_lines = [line]
        brace_depth = line.count("{") - line.count("}")
        saw_body = "{" in line
        index += 1
        while index < len(lines) and (not saw_body or brace_depth > 0):
            body_line = lines[index]
            body_lines.append(body_line)
            saw_body = saw_body or "{" in body_line
            brace_depth += body_line.count("{") - body_line.count("}")
            index += 1

        body = "\n".join(body_lines)
        if "compiler_" not in body:
            native_count += 1

    return proc_count, native_count


def render_markdown(payload: dict[str, object]) -> str:
    lines = [
        "# Stdlib Max Gate",
        "",
        f"- status: `{payload['status']}`",
        f"- version: `{payload['version']}`",
        f"- modules: `{payload['module_count']}`",
        f"- symbols: `{payload['symbol_count']}`",
        f"- source checks: `{payload['source_check_count']}`",
        f"- native procs: `{payload['native_proc_count']}`",
        "",
        "## Families",
        "",
    ]
    for family in payload["families"]:
        lines.append(
            f"- `{family['id']}` `{family['family']}`: `{family['module']}` "
            f"procs=`{family['public_procs']}` native=`{family['native_procs']}`"
        )
    if payload["failures"]:
        lines.extend(["", "## Failures", ""])
        for failure in payload["failures"]:
            lines.append(f"- `{failure['code']}` {failure['detail']}")
    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    failures: list[Failure] = []
    REPORT_DIR.mkdir(parents=True, exist_ok=True)

    if not MATRIX_PATH.exists():
        failures.append(Failure("missing-matrix", str(MATRIX_PATH.relative_to(ROOT))))
        matrix: dict[str, object] = {"requirements": [], "version": "unknown"}
    else:
        matrix = load_json(MATRIX_PATH)  # type: ignore[assignment]

    if not MODULES_PATH.exists():
        failures.append(Failure("missing-module-manifest", str(MODULES_PATH.relative_to(ROOT))))
        module_manifest: dict[str, object] = {
            "official_entrypoints": [],
            "public_module_coverage": {},
        }
    else:
        module_manifest = load_json(MODULES_PATH)  # type: ignore[assignment]

    official = set(module_manifest.get("official_entrypoints", []))
    coverage = module_manifest.get("public_module_coverage", {})
    if not isinstance(coverage, dict):
        failures.append(Failure("invalid-public-module-coverage", "public_module_coverage must be an object"))
        coverage = {}

    families: list[dict[str, str]] = []
    source_check_count = 0
    symbol_count = 0
    native_proc_count = 0
    seen_modules: set[str] = set()

    implementation_requirements = matrix.get("implementation_requirements", {})
    if not isinstance(implementation_requirements, dict):
        implementation_requirements = {}
    minimum_source_lines = int(implementation_requirements.get("minimum_source_lines", 30))
    minimum_public_procs = int(implementation_requirements.get("minimum_public_procs", 10))
    minimum_native_procs = int(implementation_requirements.get("minimum_native_procs", 8))

    requirements = matrix.get("requirements", [])
    if not isinstance(requirements, list):
        failures.append(Failure("invalid-requirements", "requirements must be an array"))
        requirements = []

    for item in requirements:
        if not isinstance(item, dict):
            failures.append(Failure("invalid-requirement", "requirement entries must be objects"))
            continue
        req_id = str(item.get("id", "unknown"))
        family = str(item.get("family", "unknown"))
        module = str(item.get("module", ""))
        symbols = item.get("symbols", [])
        if not isinstance(symbols, list):
            failures.append(Failure("invalid-symbols", f"{req_id} symbols must be an array"))
            symbols = []

        symbol_count += len(symbols)
        seen_modules.add(module)

        module_path = ROOT / module
        if not module_path.exists():
            failures.append(Failure("missing-module", f"{req_id} {module}"))
            continue

        source = module_path.read_text(encoding="utf-8")
        source_lines = len(source.splitlines())
        public_procs, native_procs = proc_implementation_counts(source)
        native_proc_count += native_procs
        families.append({
            "id": req_id,
            "family": family,
            "module": module,
            "public_procs": public_procs,
            "native_procs": native_procs,
            "source_lines": source_lines,
        })
        if source_lines < minimum_source_lines:
            failures.append(Failure("module-too-small", f"{req_id} {module}: lines={source_lines} minimum={minimum_source_lines}"))
        if public_procs < minimum_public_procs:
            failures.append(Failure("not-enough-public-procs", f"{req_id} {module}: procs={public_procs} minimum={minimum_public_procs}"))
        if native_procs < minimum_native_procs:
            failures.append(Failure("not-enough-native-procs", f"{req_id} {module}: native={native_procs} minimum={minimum_native_procs}"))

        for symbol in symbols:
            fragment = str(symbol)
            if not require_fragment(source, fragment):
                failures.append(Failure("missing-symbol", f"{req_id} {module}: {fragment}"))

        if module not in official:
            failures.append(Failure("not-official-entrypoint", f"{req_id} {module}"))

        coverage_entry = coverage.get(module)
        if not isinstance(coverage_entry, dict):
            failures.append(Failure("missing-public-coverage", f"{req_id} {module}"))
        else:
            for key in ("example", "test"):
                rel = coverage_entry.get(key)
                if not isinstance(rel, str):
                    failures.append(Failure("invalid-public-coverage", f"{req_id} {module}: {key}"))
                    continue
                proof_path = ROOT / rel
                if not proof_path.exists():
                    failures.append(Failure("missing-public-proof", f"{req_id} {rel}"))
                    continue
                if module not in proof_path.read_text(encoding="utf-8"):
                    failures.append(Failure("public-proof-missing-module", f"{req_id} {rel}: {module}"))

        ok, output = run_vitte_check(module_path)
        source_check_count += 1
        if not ok:
            detail = output.splitlines()[-1] if output else "no compiler output"
            failures.append(Failure("vitte-check-failed", f"{req_id} {module}: {detail}"))

    for required_path in (CONTRACT_TEST, DOC_PATH):
        if not required_path.exists():
            failures.append(Failure("missing-required-artifact", str(required_path.relative_to(ROOT))))

    if MAKEFILE_PATH.exists():
        makefile = MAKEFILE_PATH.read_text(encoding="utf-8")
        if "stdlib-gate:" not in makefile or "tools/stdlib_max_gate.py" not in makefile:
            failures.append(Failure("stdlib-gate-not-integrated", "Makefile must run tools/stdlib_max_gate.py from stdlib-gate"))
    else:
        failures.append(Failure("missing-makefile", "Makefile"))

    if CONTRACT_TEST.exists():
        ok, output = run_vitte_check(CONTRACT_TEST)
        source_check_count += 1
        if not ok:
            detail = output.splitlines()[-1] if output else "no compiler output"
            failures.append(Failure("contract-check-failed", detail))

    payload = {
        "schema": "vitte.stdlib.max-gate-report",
        "schema_version": "1.0.0",
        "version": matrix.get("version", "unknown"),
        "status": "fail" if failures else "ok",
        "module_count": len(seen_modules),
        "symbol_count": symbol_count,
        "source_check_count": source_check_count,
        "native_proc_count": native_proc_count,
        "minimum_source_lines": minimum_source_lines,
        "minimum_public_procs": minimum_public_procs,
        "minimum_native_procs": minimum_native_procs,
        "families": families,
        "failures": [failure.__dict__ for failure in failures],
    }

    JSON_REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MD_REPORT.write_text(render_markdown(payload), encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"[stdlib-max-gate][error] {failure.code}: {failure.detail}", file=sys.stderr)
        return 1

    print(
        "[stdlib-max-gate] OK "
        f"requirements={len(requirements)} "
        f"modules={len(seen_modules)} "
        f"symbols={symbol_count} "
        f"report={JSON_REPORT.relative_to(ROOT)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
