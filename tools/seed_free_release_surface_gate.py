#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "seed_free_release_surface_gate.json"
REPORT_MD = REPORT_DIR / "seed_free_release_surface_gate.md"

FORBIDDEN_MARKERS = (
    "bin/vittec0",
    "vittec0.seed",
    "toolchain/seed",
    "install_seed.sh",
    "verify_seed.sh",
    "BOOTSTRAP_FULL_COMPILER",
    "vitte-bootstrap-payload",
    "bootstrap payload",
    "payload bootstrap",
    "bootstrap_payload_bridge",
    "VITTE_BOOTSTRAP_COMPILER",
    "VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE",
)

FORBIDDEN_PATHS = (
    ROOT / "bin/vittec0",
    ROOT / "toolchain/seed",
    ROOT / "toolchain/src",
    ROOT / "scripts/seed",
)

PUBLISHED_BINARIES = (
    ROOT / "bin/vitte",
    ROOT / "bin/vittec",
    ROOT / "target/release/vitte",
)

REQUIRED_GATES = (
    "selfhost_full_gate",
    "release_binary_gate",
    "selfhost_stage_compare_gate",
    "release_clean_selfhost_gate",
    "release_package_stdlib_gate",
    "stdlib_total_gate",
    "compiler_backend_surface_gate",
    "compiler_link_abi_gate",
    "selfhost_ci_regression_gate",
)

REQUIRED_REPORTS = tuple(f"target/reports/{name}.json" for name in REQUIRED_GATES) + (
    "target/reports/selfhost_stage_hashes.json",
    "target/reports/selfhost_stage_hashes.sha256",
    "target/reports/selfhost_completion.md",
    "target/selfhost_completion/selfhost_completion.json",
)

WORKFLOWS = tuple(sorted((ROOT / ".github/workflows").glob("*.yml")))
MANIFESTS = (
    ROOT / "src/vitte/compiler/tests/compiler_contract_manifest.json",
    ROOT / "src/vitte/packages/registry/registry.json",
    ROOT / "src/vitte/packages/registry/lockfile.vitte.lock",
    ROOT / "src/vitte/stdlib/modules.vitte.json",
    ROOT / "src/vitte/stdlib/entrypoint.vitte.json",
)

ACTIVE_RELEASE_SCRIPTS = (
    ROOT / "tools/selfhost_stage_gates.py",
    ROOT / "tools/selfhost_stage0_gate.py",
    ROOT / "tools/selfhost_stage_compare_gate.py",
    ROOT / "tools/release_binary_gate.py",
    ROOT / "tools/release_clean_selfhost_gate.py",
    ROOT / "tools/release_package_stdlib_gate.py",
    ROOT / "tools/compiler_backend_surface_gate.py",
    ROOT / "tools/compiler_link_abi_gate.py",
    ROOT / "tools/selfhost_ci_regression_gate.py",
    ROOT / "tools/selfhost_completion_audit.py",
    ROOT / "tools/package_registry.py",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def marker_hits(data: str | bytes) -> list[str]:
    if isinstance(data, bytes):
        text = data.decode("utf-8", errors="ignore")
    else:
        text = data
    hits: list[str] = []
    for marker in FORBIDDEN_MARKERS:
        if marker == "BOOTSTRAP_FULL_COMPILER":
            if re.search(r"(?<!E_)BOOTSTRAP_FULL_COMPILER", text):
                hits.append(marker)
            continue
        if marker in text:
            hits.append(marker)
    return hits


def scan_file(path: Path, failures: list[str], scanned: list[dict[str, Any]], *, binary: bool = False) -> None:
    if not path.is_file():
        failures.append(f"missing release surface file: {rel(path)}")
        scanned.append({"path": rel(path), "missing": True})
        return
    data: str | bytes = path.read_bytes() if binary else path.read_text(encoding="utf-8", errors="replace")
    hits = marker_hits(data)
    scanned.append({"path": rel(path), "sha256": sha256_file(path), "forbidden_hits": hits})
    for marker in hits:
        failures.append(f"{rel(path)} contains active forbidden release marker: {marker}")


def make_target_body(text: str, target: str) -> str:
    match = re.search(rf"^{re.escape(target)}:.*(?:\n(?:\t.*|[^\S\n].*)?)*", text, flags=re.MULTILINE)
    return "" if match is None else match.group(0)


def scan_makefile(failures: list[str], scanned: list[dict[str, Any]]) -> None:
    path = ROOT / "Makefile"
    text = path.read_text(encoding="utf-8", errors="replace")
    targets = (
        "seed-free-release-gate",
        "release-check",
        "release-binary-gate",
        "package-registry-gate",
        "release-package-stdlib-gate",
    )
    target_hits: dict[str, list[str]] = {}
    for target in targets:
        body = make_target_body(text, target)
        hits = marker_hits(body)
        target_hits[target] = hits
        for marker in hits:
            failures.append(f"Makefile target {target} contains forbidden release marker: {marker}")
    scanned.append({"path": "Makefile", "targets": target_hits, "sha256": sha256_file(path)})


def collect_json_strings(value: Any, key: str = "") -> list[str]:
    values: list[str] = []
    if isinstance(value, dict):
        for child_key, child_value in value.items():
            values.extend(collect_json_strings(child_value, str(child_key)))
    elif isinstance(value, list):
        for item in value:
            values.extend(collect_json_strings(item, key))
    elif isinstance(value, str):
        active_keys = ("command", "args", "path", "output", "compiler", "binary", "source", "artifact")
        if any(part in key.lower() for part in active_keys):
            values.append(value)
    return values


def scan_report(path: Path, failures: list[str], scanned: list[dict[str, Any]]) -> None:
    if not path.is_file():
        failures.append(f"missing required gate report: {rel(path)}")
        scanned.append({"path": rel(path), "missing": True})
        return
    if path.suffix == ".json":
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            failures.append(f"invalid required report JSON {rel(path)}: {exc}")
            return
        values = collect_json_strings(payload)
        hits = sorted({marker for value in values for marker in marker_hits(value)})
    else:
        text = path.read_text(encoding="utf-8", errors="replace")
        hits = marker_hits(text)
    scanned.append({"path": rel(path), "sha256": sha256_file(path), "forbidden_hits": hits})
    for marker in hits:
        failures.append(f"{rel(path)} has active forbidden report marker: {marker}")


def run_git_grep() -> list[str]:
    pattern = "|".join(re.escape(marker) for marker in FORBIDDEN_MARKERS)
    proc = subprocess.run(
        ["git", "grep", "-n", "-I", "-E", pattern, "--", "bin", "Makefile", ".github/workflows", "src/vitte", "target/reports"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return [line for line in proc.stdout.splitlines() if line]


def validate_ci(failures: list[str]) -> dict[str, Any]:
    workflow = ROOT / ".github/workflows/compiler-vitte-chain.yml"
    text = workflow.read_text(encoding="utf-8")
    checks = {
        "runs_seed_free_release_gate": "make seed-free-release-gate" in text,
        "uploads_selfhost_reports": "target/reports/selfhost_*_gate.json" in text,
        "uploads_stage_hashes_json": "target/reports/selfhost_stage_hashes.json" in text,
        "uploads_stage_hashes_sha256": "target/reports/selfhost_stage_hashes.sha256" in text,
        "artifact_missing_is_error": "if-no-files-found: error" in text,
        "uploads_surface_gate_report": "target/reports/seed_free_release_surface_gate.json" in text,
    }
    for name, ok in checks.items():
        if not ok:
            failures.append(f"CI missing required seed-free release behavior: {name}")
    return checks


def main() -> int:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    failures: list[str] = []
    scanned: list[dict[str, Any]] = []

    for path in FORBIDDEN_PATHS:
        exists = path.exists()
        scanned.append({"path": rel(path), "forbidden_path_exists": exists})
        if exists:
            failures.append(f"forbidden seed/bootstrap path exists: {rel(path)}")

    for path in PUBLISHED_BINARIES:
        scan_file(path, failures, scanned, binary=True)
    scan_makefile(failures, scanned)
    for script in ACTIVE_RELEASE_SCRIPTS:
        scan_file(script, failures, scanned)
    for workflow in WORKFLOWS:
        scan_file(workflow, failures, scanned)
    for manifest in MANIFESTS:
        scan_file(manifest, failures, scanned)
    for report in REQUIRED_REPORTS:
        scan_report(ROOT / report, failures, scanned)

    ci_checks = validate_ci(failures)
    git_hits = run_git_grep()
    status = "fail" if failures else "pass"
    payload = {
        "schema": "vitte.seed_free.release.surface.gate.v1",
        "status": status,
        "forbidden_markers": FORBIDDEN_MARKERS,
        "required_gates": REQUIRED_GATES,
        "required_reports": REQUIRED_REPORTS,
        "ci_checks": ci_checks,
        "scanned": scanned,
        "repository_marker_observations": len(git_hits),
        "repository_marker_policy": "seed/bootstrap paths are removed; active release surfaces must not reference or recreate them",
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT_MD.write_text(
        "# seed-free release surface gate\n\n"
        f"- status: {status}\n"
        f"- scanned files: {len(scanned)}\n"
        f"- required gates: {len(REQUIRED_GATES)}\n"
        f"- required reports: {len(REQUIRED_REPORTS)}\n"
        f"- repository marker observations: {len(git_hits)}\n"
        + ("\n## Failures\n" + "\n".join(f"- {failure}" for failure in failures) + "\n" if failures else ""),
        encoding="utf-8",
    )
    if failures:
        print("[seed-free-release-surface-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("[seed-free-release-surface-gate] ok binaries=scripts+makefile+ci+manifests+reports seed=absent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
