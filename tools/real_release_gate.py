#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "docs" / "release" / "installer_real_platforms.json"
REPORT_JSON = ROOT / "target" / "reports" / "real_release_gate.json"
REPORT_MD = ROOT / "target" / "reports" / "real_release_gate.md"
COMPILE_ALL_SUMMARY = ROOT / "target" / "reports" / "compiler_compile_all" / "summary.txt"
COMPILE_ALL_BUILD_OK = ROOT / "target" / "reports" / "compiler_compile_all" / "build_ok.txt"
INSTALLER_WORKFLOW = ROOT / ".github" / "workflows" / "installer-real-platforms.yml"
RELEASE_WORKFLOW = ROOT / ".github" / "workflows" / "release-stability-gate.yml"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def binary_format(path: Path) -> str:
    data = path.read_bytes()[:4096]
    if data.startswith(b"\x7fELF"):
        return "ELF"
    if data.startswith(b"MZ"):
        pe_offset = int.from_bytes(data[0x3C:0x40], "little", signed=False) if len(data) >= 0x40 else 0
        if pe_offset > 0 and len(data) >= pe_offset + 4 and data[pe_offset:pe_offset + 4] == b"PE\0\0":
            return "PE"
        return "PE"
    if data[:4] in {
        b"\xfe\xed\xfa\xce",
        b"\xfe\xed\xfa\xcf",
        b"\xce\xfa\xed\xfe",
        b"\xcf\xfa\xed\xfe",
        b"\xca\xfe\xba\xbe",
        b"\xca\xfe\xba\xbf",
    }:
        return "Mach-O"
    if data.startswith(b"#!"):
        return "script"
    return "unknown"


def parse_summary(path: Path) -> dict[str, int]:
    values: dict[str, int] = {}
    for line in read(path).splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        try:
            values[key.strip()] = int(value.strip())
        except ValueError:
            continue
    return values


def command_text(command: object) -> str:
    if isinstance(command, list):
        return " ".join(str(part) for part in command)
    return str(command)


def command_basename(command: object) -> str:
    if isinstance(command, list) and command:
        return Path(str(command[0])).name.lower()
    return Path(str(command).split(" ", 1)[0]).name.lower()


def target_key(target: dict[str, str]) -> tuple[str, str]:
    return target["os"], target["arch"]


def strict_artifact_path(root: Path, os_name: str, arch: str) -> Path:
    suffix = ".exe" if os_name.startswith("windows") else ""
    return root / f"{os_name}-{arch}" / f"vitte{suffix}"


def check_real_binary(
    *,
    target: dict[str, str],
    artifact_root: Path,
    expected_formats: dict[str, str],
    failures: list[str],
) -> dict[str, object]:
    os_name = target["os"]
    arch = target["arch"]
    path = strict_artifact_path(artifact_root, os_name, arch)
    expected = expected_formats.get(os_name, "ELF")
    row: dict[str, object] = {
        "target": target,
        "artifact": str(path.relative_to(ROOT)),
        "expected_format": expected,
        "present": path.exists() and path.stat().st_size > 0,
        "format": "",
        "attestation": str((path.parent / "ATTESTATION.json").relative_to(ROOT)),
        "smoke_has_version": False,
        "smoke_has_help": False,
        "smoke_has_check": False,
        "smoke_has_build": False,
        "smoke_has_run": False,
    }
    if not row["present"]:
        failures.append(f"missing real binary {path.relative_to(ROOT)}")
        return row

    actual = binary_format(path)
    row["format"] = actual
    if actual != expected:
        failures.append(f"wrong binary format {path.relative_to(ROOT)}: expected {expected}, got {actual}")

    attestation_path = path.parent / "ATTESTATION.json"
    if not attestation_path.exists():
        failures.append(f"missing real binary attestation {attestation_path.relative_to(ROOT)}")
        return row
    try:
        attestation = json.loads(attestation_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        failures.append(f"invalid attestation JSON {attestation_path.relative_to(ROOT)}: {exc}")
        return row

    if attestation.get("schema") != "org.vitte.real-binary-attestation.v1":
        failures.append(f"wrong attestation schema {attestation_path.relative_to(ROOT)}")
    if attestation.get("target") != {"os": os_name, "arch": arch}:
        failures.append(f"attestation target mismatch {attestation_path.relative_to(ROOT)}")
    if attestation.get("format") != expected:
        failures.append(f"attestation format mismatch {attestation_path.relative_to(ROOT)}")
    if attestation.get("sha256") != sha256(path):
        failures.append(f"attestation checksum mismatch {attestation_path.relative_to(ROOT)}")
    if int(attestation.get("size", -1)) != path.stat().st_size:
        failures.append(f"attestation size mismatch {attestation_path.relative_to(ROOT)}")

    commands = attestation.get("smoke_commands", [])
    if not isinstance(commands, list) or not commands:
        failures.append(f"attestation has no smoke command evidence {attestation_path.relative_to(ROOT)}")
        return row
    texts = [command_text(entry.get("command", "")) for entry in commands if isinstance(entry, dict)]
    exits_ok = all(isinstance(entry, dict) and entry.get("exit_code") == 0 for entry in commands)
    row["smoke_has_version"] = any("--version" in text for text in texts)
    row["smoke_has_help"] = any("--help" in text for text in texts)
    row["smoke_has_check"] = any(" check " in f" {text} " and "smoke.vit" in text for text in texts)
    row["smoke_has_build"] = any(" build " in f" {text} " and "smoke.vit" in text for text in texts)
    row["smoke_has_run"] = any(command_basename(entry.get("command", "")) in {"smoke", "smoke.exe"} for entry in commands if isinstance(entry, dict))
    if not exits_ok:
        failures.append(f"attestation contains failing smoke command {attestation_path.relative_to(ROOT)}")
    for key in ("smoke_has_version", "smoke_has_help", "smoke_has_check", "smoke_has_build", "smoke_has_run"):
        if not row[key]:
            failures.append(f"attestation missing {key.removeprefix('smoke_has_')} evidence {attestation_path.relative_to(ROOT)}")
    return row


def check_native_entrypoints(matrix: dict[str, object], failures: list[str]) -> dict[str, object]:
    summary = parse_summary(COMPILE_ALL_SUMMARY)
    build_ok = set(read(COMPILE_ALL_BUILD_OK).splitlines())
    entrypoints = [str(item) for item in matrix.get("native_compiler_entrypoints", [])]
    rows = []
    if not summary:
        failures.append(f"missing compiler compile-all summary {COMPILE_ALL_SUMMARY.relative_to(ROOT)}")
    if summary.get("check_fail", 1) != 0:
        failures.append("compiler compile-all has check failures")
    if summary.get("build_native_fail", 1) != 0:
        failures.append("compiler compile-all has native build failures")
    if summary.get("build_native_ok", 0) < len(entrypoints):
        failures.append(f"compiler native entrypoint builds too low: {summary.get('build_native_ok', 0)} < {len(entrypoints)}")
    for entrypoint in entrypoints:
        source = ROOT / entrypoint
        built = entrypoint in build_ok
        rows.append({"entrypoint": entrypoint, "source_present": source.exists(), "native_build_ok": built})
        if not source.exists():
            failures.append(f"missing compiler entrypoint source {entrypoint}")
        if entrypoint not in build_ok:
            failures.append(f"compiler entrypoint was not natively built {entrypoint}")
    return {"summary": summary, "entrypoints": rows}


def check_real_ci(matrix: dict[str, object], failures: list[str]) -> dict[str, object]:
    installer_workflow = read(INSTALLER_WORKFLOW)
    release_workflow = read(RELEASE_WORKFLOW)
    targets = matrix.get("binary_ci_targets", []) + matrix.get("raspberry_pi_targets", []) + matrix.get("real_platform_targets", [])
    labels = []
    for target in targets:
        label = str(target.get("runner_label", ""))
        labels.append(label)
        if label not in installer_workflow:
            failures.append(f"installer workflow missing real runner label {label}")
    for term in ("self-hosted", "scripts/ci/real-install-smoke.sh", "scripts\\ci\\real-install-smoke.cmd", "actions/upload-artifact", "if-no-files-found: error", "make stage-real-binary"):
        if term not in installer_workflow:
            failures.append(f"installer workflow missing `{term}`")
    for term in ("STRICT_REAL_INSTALLERS=1", "RELEASE_INSTALLER_GATE=1", "make real-release-gate"):
        if term not in release_workflow:
            failures.append(f"release workflow missing `{term}`")
    return {"labels": labels, "installer_workflow": str(INSTALLER_WORKFLOW.relative_to(ROOT)), "release_workflow": str(RELEASE_WORKFLOW.relative_to(ROOT))}


def run_verify_installers(installer_root: Path, failures: list[str]) -> dict[str, object]:
    command = ["sh", "scripts_build/verify-installers.sh"]
    env = os.environ.copy()
    env.update({"OUT_DIR": str(installer_root), "VERIFY_METADATA": "1", "RELEASE_INSTALLER_GATE": "1"})
    completed = subprocess.run(command, cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if completed.returncode != 0:
        failures.append("strict installer verification failed")
    return {"command": command, "exit_code": completed.returncode, "output": completed.stdout[-4000:]}


def check_installer_artifacts(matrix: dict[str, object], failures: list[str]) -> dict[str, object]:
    installer_root = ROOT / str(matrix.get("strict_installer_artifact_root", "pkgout"))
    required_metadata = [str(item) for item in matrix.get("artifact_metadata_required", [])]
    required_artifacts = [dict(item) for item in matrix.get("strict_installer_artifacts", [])]
    metadata_rows = []
    artifact_rows = []
    if not installer_root.exists():
        failures.append(f"missing strict installer artifact root {installer_root.relative_to(ROOT)}")
    for name in required_metadata:
        if "*" in name:
            matches = sorted(installer_root.glob(name)) if installer_root.exists() else []
            present = bool(matches)
        else:
            present = (installer_root / name).is_file()
        metadata_rows.append({"name": name, "present": present})
        if not present:
            failures.append(f"missing strict installer metadata {installer_root.relative_to(ROOT) / name}")
    for spec in required_artifacts:
        pattern = str(spec["pattern"])
        matches = sorted(installer_root.glob(pattern)) if installer_root.exists() else []
        artifact_rows.append({"family": spec.get("family"), "target": spec.get("target"), "pattern": pattern, "matches": [str(path.relative_to(ROOT)) for path in matches]})
        if not matches:
            failures.append(f"missing strict installer artifact {pattern}")
            continue
        for path in matches:
            manifest = installer_root / f"{path.name}.MANIFEST.json"
            if not manifest.is_file():
                failures.append(f"missing artifact manifest {manifest.relative_to(ROOT)}")
    verify = run_verify_installers(installer_root, failures) if installer_root.exists() else {"command": ["sh", "scripts_build/verify-installers.sh"], "exit_code": None, "output": "skipped: installer root missing"}
    return {
        "root": str(installer_root.relative_to(ROOT)),
        "metadata": metadata_rows,
        "artifacts": artifact_rows,
        "verify": verify,
    }


def write_reports(report: dict[str, object]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# Real Release Gate",
        "",
        f"Status: {report['status']}",
        f"Failures: {len(report['failures'])}",
        "",
        "## Requirements",
        "",
        "- real multi-architecture binaries",
        "- native compiler entrypoint builds",
        "- strict CI on real machines or VMs",
        "- strict installer artifacts and metadata",
        "- post-install `check + build + run` evidence",
        "",
        "## Failures",
        "",
    ]
    failures = report["failures"]
    if isinstance(failures, list) and failures:
        lines.extend(f"- {failure}" for failure in failures)
    else:
        lines.append("- none")
    REPORT_MD.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Enforce the real multi-arch release gate")
    parser.add_argument("--allow-pending", action="store_true", help="write reports but return 0 while real artifacts are still pending")
    args = parser.parse_args(argv)

    matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
    failures: list[str] = []
    artifact_root = ROOT / str(matrix.get("strict_artifact_root", "target/real-binaries"))
    expected_formats = dict(matrix.get("real_binary_formats", {}))
    targets = matrix.get("binary_ci_targets", []) + matrix.get("raspberry_pi_targets", []) + matrix.get("real_platform_targets", [])
    unique_targets = list({target_key(dict(target)): dict(target) for target in targets}.values())

    real_binaries = [
        check_real_binary(target=target, artifact_root=artifact_root, expected_formats=expected_formats, failures=failures)
        for target in unique_targets
    ]
    native_entrypoints = check_native_entrypoints(matrix, failures)
    real_ci = check_real_ci(matrix, failures)
    installers = check_installer_artifacts(matrix, failures)

    report = {
        "schema": "org.vitte.real-release-gate.v1",
        "version": matrix.get("version", "0.1.0"),
        "status": "PASS" if not failures else "PENDING" if args.allow_pending else "FAIL",
        "allow_pending": args.allow_pending,
        "real_binary_targets": real_binaries,
        "native_entrypoints": native_entrypoints,
        "real_ci": real_ci,
        "installers": installers,
        "failures": failures,
    }
    write_reports(report)

    if failures and not args.allow_pending:
        for failure in failures:
            print(f"[real-release-gate][error] {failure}", file=sys.stderr)
        print(f"[real-release-gate] report={REPORT_JSON.relative_to(ROOT)}", file=sys.stderr)
        return 1

    print(f"[real-release-gate] {report['status']} report={REPORT_JSON.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
