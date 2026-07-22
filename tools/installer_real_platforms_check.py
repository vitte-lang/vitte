#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "docs/release/installer_real_platforms.json"
REPORT = ROOT / "target/reports/installers/real-platforms.json"
PACKAGE_MATRIX = ROOT / "scripts/package-matrix.sh"
WORKFLOW = ROOT / ".github/workflows/installer-real-platforms.yml"
POSIX_SMOKE = ROOT / "scripts/ci/real-install-smoke.sh"
WINDOWS_SMOKE = ROOT / "scripts/ci/real-install-smoke.ps1"
WINDOWS_CMD_SMOKE = ROOT / "scripts/ci/real-install-smoke.cmd"


ARCH_ALIASES = {
    "x86_64": ("linux", "x86_64", "deb"),
    "i386": ("linux", "i386", "deb"),
    "arm64": ("linux", "arm64", "deb"),
    "armv7": ("linux", "armv7", "deb"),
    "armv6": ("linux", "armv6", "deb"),
    "riscv64": ("linux", "riscv64", "deb"),
}


def run_ok(args: list[str]) -> bool:
    return subprocess.run(args, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def strict_artifact_path(root: Path, os_name: str, arch: str) -> Path:
    suffix = ".exe" if os_name.startswith("windows") else ""
    return root / f"{os_name}-{arch}" / f"vitte{suffix}"


def command_present(text: str, command: str, shell: str) -> bool:
    if command in text:
        return True
    suffix = command.removeprefix("vitte")
    if shell == "posix":
        return f"$VITTE_BIN{suffix}" in text or f'"$VITTE_BIN"{suffix}' in text
    if shell == "windows":
        return f"$VitteBin{suffix}" in text or f"%VITTE_BIN%{suffix}" in text or f'"%VITTE_BIN%"{suffix}' in text
    return False


def main() -> int:
    strict = os.environ.get("STRICT_REAL_INSTALLERS", "0") == "1"
    release_mode = (
        os.environ.get("RELEASE_INSTALLER_GATE", "0") == "1"
        or os.environ.get("GITHUB_REF_TYPE") == "tag"
    )
    data = json.loads(MATRIX.read_text(encoding="utf-8"))
    failures: list[str] = []
    warnings: list[str] = []

    if data.get("release_requires_strict_real_installers") and release_mode and not strict:
        failures.append("release installer gate requires STRICT_REAL_INSTALLERS=1")

    for arch in data["required_binary_architectures"]:
        os_name, matrix_arch, fmt = ARCH_ALIASES[arch]
        if not run_ok([str(PACKAGE_MATRIX), "lookup", os_name, matrix_arch, fmt]):
            failures.append(f"missing package matrix support for {arch}")

    raspberry_arches = {target["arch"] for target in data["raspberry_pi_targets"]}
    for arch in ("armv6", "armv7", "aarch64"):
        if arch not in raspberry_arches:
            failures.append(f"missing Raspberry Pi target {arch}")

    workflow_text = read(WORKFLOW)
    binary_ci_arches = {target["arch"] for target in data.get("binary_ci_targets", [])}
    for arch in data["required_binary_architectures"]:
        if arch not in binary_ci_arches:
            failures.append(f"missing binary CI target for {arch}")
    for target in data.get("binary_ci_targets", []):
        label = target.get("runner_label", "")
        if label not in workflow_text:
            failures.append(f"real platform workflow missing binary runner `{label}`")

    for required in (
        "workflow_dispatch",
        "self-hosted",
        "windows-xp-i386",
        "raspberrypi-armv6",
        "solaris-i386",
        "scripts/ci/real-install-smoke.sh",
        "scripts\\ci\\real-install-smoke.ps1",
        "scripts\\ci\\real-install-smoke.cmd",
    ):
        if required not in workflow_text:
            failures.append(f"real platform workflow missing `{required}`")

    for key in (
        "release_requires_sbom",
        "release_requires_signatures",
        "release_requires_notarization_for_macos",
        "artifact_metadata_required",
    ):
        if key not in data:
            failures.append(f"installer real platforms contract missing `{key}`")

    posix_text = read(POSIX_SMOKE)
    windows_text = read(WINDOWS_SMOKE)
    windows_cmd_text = read(WINDOWS_CMD_SMOKE)
    for command in data["post_install_commands"]:
        if not command_present(posix_text, command, "posix"):
            failures.append(f"POSIX smoke missing `{command}`")
        if not command_present(windows_text, command, "windows"):
            failures.append(f"Windows smoke missing `{command}`")
        if not command_present(windows_cmd_text, command, "windows"):
            failures.append(f"Windows cmd smoke missing `{command}`")

    artifact_root = ROOT / data["strict_artifact_root"]
    strict_rows = []
    strict_targets = data.get("binary_ci_targets", []) + data["real_platform_targets"] + data["raspberry_pi_targets"]
    for target in strict_targets:
        path = strict_artifact_path(artifact_root, target["os"], target["arch"])
        present = path.exists() and path.stat().st_size > 0
        strict_rows.append({"target": target, "artifact": str(path.relative_to(ROOT)), "present": present})
        if strict and not present:
            failures.append(f"missing strict real binary artifact {path.relative_to(ROOT)}")
        elif not present:
            warnings.append(f"pending strict artifact {path.relative_to(ROOT)}")

    report = {
        "schema": data["schema"],
        "version": data["version"],
        "status": "PASS" if not failures else "FAIL",
        "strict": strict,
        "release_mode": release_mode,
        "release_requires_strict_real_installers": bool(data.get("release_requires_strict_real_installers")),
        "required_binary_architectures": data["required_binary_architectures"],
        "binary_ci_targets": data.get("binary_ci_targets", []),
        "raspberry_pi_targets": data["raspberry_pi_targets"],
        "real_platform_targets": data["real_platform_targets"],
        "post_install_commands": data["post_install_commands"],
        "artifact_metadata_required": data.get("artifact_metadata_required", []),
        "strict_artifacts": strict_rows,
        "warnings": warnings,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"[installer-real-platforms][error] {failure}", file=sys.stderr)
        print(f"[installer-real-platforms] report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1

    print(
        f"[installer-real-platforms] OK strict={int(strict)} "
        f"targets={len(strict_targets)} "
        f"report={REPORT.relative_to(ROOT)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
