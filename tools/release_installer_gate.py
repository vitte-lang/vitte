#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "docs" / "release" / "installer_real_platforms.json"
REPORT = ROOT / "target" / "reports" / "installers" / "release-installer-gate.json"

REQUIRED_ARCHES = {"x86_64", "i386", "arm64", "armv7", "armv6", "riscv64"}
REQUIRED_BUILD_FAMILIES = {"linux", "portable", "freebsd", "bsd", "macos", "solaris", "windows"}
REQUIRED_METADATA = {
    "INSTALLERS.json",
    "CHECKSUMS.txt",
    "SIGNATURES.json",
    "SBOM.spdx.json",
    "SBOM.cyclonedx.json",
    "*.MANIFEST.json",
}
REQUIRED_SMOKE_TERMS = {
    "vitte-installer-doctor",
    "--version",
    "--help",
    "check smoke.vit",
    "build smoke.vit -o smoke",
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def run_ok(args: list[str]) -> bool:
    return subprocess.run(args, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0


def fail_if_missing_terms(path: Path, terms: set[str], failures: list[str]) -> None:
    text = read(path)
    for term in sorted(terms):
        if term not in text:
            failures.append(f"{path.relative_to(ROOT)} missing `{term}`")


def main() -> int:
    release_mode = os.environ.get("RELEASE_INSTALLER_GATE", "0") == "1" or os.environ.get("GITHUB_REF_TYPE") == "tag"
    strict = os.environ.get("STRICT_REAL_INSTALLERS", "0") == "1"
    matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
    failures: list[str] = []

    arches = set(matrix.get("required_binary_architectures", []))
    if arches != REQUIRED_ARCHES:
        failures.append(f"required_binary_architectures must be {sorted(REQUIRED_ARCHES)}, got {sorted(arches)}")

    binary_ci_arches = {target.get("arch") for target in matrix.get("binary_ci_targets", [])}
    for arch in REQUIRED_ARCHES:
        if arch not in binary_ci_arches:
            failures.append(f"missing binary CI target for {arch}")

    metadata = set(matrix.get("artifact_metadata_required", []))
    if not REQUIRED_METADATA.issubset(metadata):
        failures.append(f"artifact metadata policy missing {sorted(REQUIRED_METADATA - metadata)}")
    if "ATTESTATION.json" not in metadata:
        failures.append("artifact metadata policy missing real binary attestation")
    if "real_binary_formats" not in matrix:
        failures.append("installer real platforms contract missing real_binary_formats")

    if release_mode and not strict:
        failures.append("release installer gate requires STRICT_REAL_INSTALLERS=1")
    if release_mode and not matrix.get("release_requires_sbom"):
        failures.append("release installer gate requires release_requires_sbom=true")
    if release_mode and not matrix.get("release_requires_signatures"):
        failures.append("release installer gate requires release_requires_signatures=true")
    if release_mode and not matrix.get("release_requires_notarization_for_macos"):
        failures.append("release installer gate requires release_requires_notarization_for_macos=true")

    build_all = ROOT / "scripts_build" / "build-all-installers.sh"
    build_all_text = read(build_all)
    for family in REQUIRED_BUILD_FAMILIES:
        if f"all | {family}" not in build_all_text:
            failures.append(f"build-all-installers missing family {family}")
    for term in ("SBOM.spdx.json", "SBOM.cyclonedx.json", "SIGNATURES.json", ".MANIFEST.json", "SOURCE_DATE_EPOCH"):
        if term not in build_all_text:
            failures.append(f"build-all-installers missing `{term}`")

    verify = ROOT / "scripts_build" / "verify-installers.sh"
    verify_text = read(verify)
    for term in ("INSTALLERS.json", "CHECKSUMS.txt", "SIGNATURES.json", "SBOM.spdx.json", "SBOM.cyclonedx.json", "MANIFEST.json"):
        if term not in verify_text:
            failures.append(f"verify-installers missing `{term}`")

    package_matrix = ROOT / "scripts_build" / "package-matrix.sh"
    for arch in ("amd64", "i386", "arm64", "armv7", "armv6", "riscv64"):
        if not run_ok([str(package_matrix), "lookup", "linux", arch, "deb"]):
            failures.append(f"package matrix missing linux {arch} deb")
    for arch in ("amd64", "i386", "arm64", "armv7", "armv6", "riscv64"):
        if arch not in build_all_text and arch not in read(ROOT / "scripts_build" / "build-portable-tarball.sh"):
            failures.append(f"portable build scripts missing {arch}")

    for smoke in (
        ROOT / "scripts" / "ci" / "real-install-smoke.sh",
        ROOT / "scripts" / "ci" / "real-install-smoke.cmd",
        ROOT / "scripts" / "ci" / "real-install-smoke.ps1",
    ):
        fail_if_missing_terms(smoke, REQUIRED_SMOKE_TERMS, failures)

    doctor = ROOT / "scripts_build" / "installer-doctor.sh"
    fail_if_missing_terms(doctor, REQUIRED_SMOKE_TERMS, failures)

    stage = ROOT / "scripts_build" / "stage-installer-payload.sh"
    fail_if_missing_terms(stage, {"vitte-installer-doctor.cmd", "--version", "--help", "check smoke.vit", "build smoke.vit -o smoke"}, failures)

    workflow = ROOT / ".github" / "workflows" / "installer-real-platforms.yml"
    workflow_text = read(workflow)
    for target in matrix.get("binary_ci_targets", []) + matrix.get("raspberry_pi_targets", []) + matrix.get("real_platform_targets", []):
        label = target.get("runner_label", "")
        if label and label not in workflow_text:
            failures.append(f"installer real platform workflow missing runner {label}")
    if "scripts/ci/real-install-smoke.sh" not in workflow_text or "scripts\\ci\\real-install-smoke.cmd" not in workflow_text:
        failures.append("installer real platform workflow missing smoke scripts")
    for term in ("make stage-real-binary", "tools\\stage_real_binary.py", "actions/upload-artifact", "target/real-binaries"):
        if term not in workflow_text:
            failures.append(f"installer real platform workflow missing `{term}`")

    release_workflow = read(ROOT / ".github" / "workflows" / "release-stability-gate.yml")
    for term in ("STRICT_REAL_INSTALLERS=1", "RELEASE_INSTALLER_GATE=1", "make release-installer-gate"):
        if term not in release_workflow:
            failures.append(f"release workflow missing `{term}`")

    stage_real = ROOT / "tools" / "stage_real_binary.py"
    fail_if_missing_terms(stage_real, {"binary_format", "ATTESTATION.json", "smoke_commands", "ELF", "Mach-O", "PE"}, failures)
    fail_if_missing_terms(ROOT / "docs" / "release" / "real_binary_collection.md", {"make stage-real-binary", "STRICT_REAL_INSTALLERS=1", "ATTESTATION.json"}, failures)

    report = {
        "schema": "org.vitte.release-installer-gate.v1",
        "status": "PASS" if not failures else "FAIL",
        "release_mode": release_mode,
        "strict": strict,
        "required_arches": sorted(REQUIRED_ARCHES),
        "required_build_families": sorted(REQUIRED_BUILD_FAMILIES),
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"[release-installer-gate][error] {failure}", file=sys.stderr)
        print(f"[release-installer-gate] report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[release-installer-gate] OK report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
