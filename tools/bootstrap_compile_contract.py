#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
COMPILER_ROOT = ROOT / "src/vitte/compiler"
COMPILER_ENTRY = COMPILER_ROOT / "main.vit"
STAGE0_MANIFEST = ROOT / "toolchain/bootstrap/stage0-manifest.json"
STAGES_MANIFEST = ROOT / "toolchain/bootstrap/stages-manifest.json"
BOOTSTRAP_REAL = ROOT / "tools/bootstrap_real/bootstrap_real.py"
REPORT_JSON = ROOT / "target/reports/bootstrap_compile_contract.json"
REPORT_MD = ROOT / "target/reports/bootstrap_compile_contract.md"

EXPECTED_COMPILER_MODULES = 971
EXPECTED_STAGE0_SCHEMA = "vitte.bootstrap.stage0.trust.v1"
EXPECTED_STAGES_SCHEMA = "vitte.toolchain.bootstrap.stages.v2"
EXPECTED_CHAIN = [
    "signed_stage0",
    "trusted_stage0",
    "bootstrap_compiler",
    "stage1",
    "stage2",
    "release",
    "bin",
    "vittec",
]
EXPECTED_STAGE_BUILDERS = {
    "stage1_build_command": "bootstrap_compiler",
    "stage2_build_command": "stage1",
    "release_build_command": "stage2",
}
HIGH_RISK_BINARY_MARKERS = (
    b"vitte-bootstrap-payload",
    b"payload_source",
    b"write_payload_file",
    b"BOOTSTRAP_FULL_COMPILER",
    b"VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def repo_path(raw: object, field: str, errors: list[str]) -> Path | None:
    if not isinstance(raw, str) or not raw:
        errors.append(f"{field} must be a non-empty repository-relative path")
        return None
    path = (ROOT / raw).resolve()
    try:
        path.relative_to(ROOT)
    except ValueError:
        errors.append(f"{field} escapes repository: {raw}")
        return None
    return path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def binary_format(path: Path) -> str:
    data = path.read_bytes()[:8]
    if data.startswith(b"\x7fELF"):
        return "ELF"
    if data.startswith(b"MZ"):
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


def load_json(path: Path, errors: list[str]) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        errors.append(f"missing JSON file: {rel(path)}")
        return {}
    except json.JSONDecodeError as exc:
        errors.append(f"invalid JSON in {rel(path)}: {exc}")
        return {}
    if not isinstance(data, dict):
        errors.append(f"{rel(path)} must contain a JSON object")
        return {}
    return data


def verify_signature(artifact: Path, signature: Path, public_key: Path, errors: list[str]) -> bool:
    openssl = shutil.which("openssl")
    if not openssl:
        errors.append("openssl is required to verify signed stage0 artifacts")
        return False
    completed = subprocess.run(
        [openssl, "dgst", "-sha256", "-verify", str(public_key), "-signature", str(signature), str(artifact)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if completed.returncode != 0 or "Verified OK" not in completed.stdout:
        errors.append(f"stage0 signature verification failed for {rel(artifact)}: {completed.stdout.strip()}")
        return False
    return True


def compiler_sources() -> list[Path]:
    return sorted(path for path in COMPILER_ROOT.rglob("*.vit") if path.is_file())


def check_compiler_surface(errors: list[str]) -> dict[str, Any]:
    sources = compiler_sources()
    entry_text = COMPILER_ENTRY.read_text(encoding="utf-8", errors="replace") if COMPILER_ENTRY.is_file() else ""
    if len(sources) != EXPECTED_COMPILER_MODULES:
        errors.append(f"compiler module count changed: expected {EXPECTED_COMPILER_MODULES}, got {len(sources)}")
    if "use vitte/compiler/driver/compiler.{ run_cli_main_with_ice_boundary }" not in entry_text:
        errors.append("compiler main.vit no longer imports run_cli_main_with_ice_boundary")
    if "run_cli_main_with_ice_boundary(args)" not in entry_text:
        errors.append("compiler main.vit no longer calls run_cli_main_with_ice_boundary(args)")
    if 'COMPILER_SOURCE_ROOT: string = "src/vitte/compiler"' not in entry_text:
        errors.append("compiler main.vit no longer declares the canonical compiler source root")
    return {
        "entrypoint": rel(COMPILER_ENTRY),
        "module_count": len(sources),
        "expected_module_count": EXPECTED_COMPILER_MODULES,
        "ice_boundary": "run_cli_main_with_ice_boundary(args)" in entry_text,
    }


def check_stage0_manifest(errors: list[str]) -> dict[str, Any]:
    data = load_json(STAGE0_MANIFEST, errors)
    if not data:
        return {}
    if data.get("schema") != EXPECTED_STAGE0_SCHEMA:
        errors.append(f"{rel(STAGE0_MANIFEST)} schema must be {EXPECTED_STAGE0_SCHEMA}")
    if data.get("signature_algorithm") != "openssl-rsa-sha256":
        errors.append(f"{rel(STAGE0_MANIFEST)} signature_algorithm must be openssl-rsa-sha256")
    artifacts = data.get("artifacts")
    if not isinstance(artifacts, list) or not artifacts:
        errors.append(f"{rel(STAGE0_MANIFEST)} artifacts must be a non-empty list")
        return {}

    rows: list[dict[str, Any]] = []
    seen: set[tuple[str, str]] = set()
    for index, entry in enumerate(artifacts):
        if not isinstance(entry, dict):
            errors.append(f"{rel(STAGE0_MANIFEST)} artifacts[{index}] must be an object")
            continue
        os_name = str(entry.get("os", ""))
        arch = str(entry.get("arch", ""))
        key = (os_name, arch)
        if key in seen:
            errors.append(f"duplicate stage0 artifact for {os_name}-{arch}")
        seen.add(key)
        artifact = repo_path(entry.get("artifact"), f"artifacts[{index}].artifact", errors)
        signature = repo_path(entry.get("signature"), f"artifacts[{index}].signature", errors)
        public_key = repo_path(entry.get("public_key"), f"artifacts[{index}].public_key", errors)
        row: dict[str, Any] = {"os": os_name, "arch": arch, "signature_verified": False}
        if artifact and artifact.is_file():
            actual_sha = sha256(artifact)
            row.update({"artifact": rel(artifact), "sha256": actual_sha, "format": binary_format(artifact)})
            if actual_sha != entry.get("sha256"):
                errors.append(f"{rel(artifact)} SHA-256 differs from stage0 manifest")
            if row["format"] != entry.get("format"):
                errors.append(f"{rel(artifact)} format differs from stage0 manifest")
            data_bytes = artifact.read_bytes()
            markers = [marker.decode("utf-8") for marker in HIGH_RISK_BINARY_MARKERS if marker in data_bytes]
            row["high_risk_markers"] = markers
            if markers:
                errors.append(f"{rel(artifact)} contains high-risk bootstrap payload markers: {', '.join(markers)}")
        elif artifact:
            errors.append(f"missing stage0 artifact: {rel(artifact)}")
        for label, path in (("signature", signature), ("public key", public_key)):
            if path and not path.is_file():
                errors.append(f"missing stage0 {label}: {rel(path)}")
        if artifact and signature and public_key and artifact.is_file() and signature.is_file() and public_key.is_file():
            row["signature_verified"] = verify_signature(artifact, signature, public_key, errors)
        rows.append(row)
    return {"manifest": rel(STAGE0_MANIFEST), "artifacts": rows}


def check_stages_manifest(errors: list[str]) -> dict[str, Any]:
    data = load_json(STAGES_MANIFEST, errors)
    if not data:
        return {}
    if data.get("schema") != EXPECTED_STAGES_SCHEMA:
        errors.append(f"{rel(STAGES_MANIFEST)} schema must be {EXPECTED_STAGES_SCHEMA}")
    if data.get("canonical_chain") != EXPECTED_CHAIN:
        errors.append(f"{rel(STAGES_MANIFEST)} canonical_chain must be {EXPECTED_CHAIN}")
    if data.get("source_of_truth") != rel(COMPILER_ENTRY):
        errors.append(f"{rel(STAGES_MANIFEST)} source_of_truth must be {rel(COMPILER_ENTRY)}")

    stages = data.get("stages")
    if not isinstance(stages, list):
        errors.append(f"{rel(STAGES_MANIFEST)} stages must be a list")
        return {}
    by_name: dict[str, dict[str, Any]] = {}
    rows: list[dict[str, Any]] = []
    for index, stage in enumerate(stages):
        if not isinstance(stage, dict):
            errors.append(f"{rel(STAGES_MANIFEST)} stages[{index}] must be an object")
            continue
        name = str(stage.get("name", ""))
        if name in by_name:
            errors.append(f"duplicate bootstrap stage: {name}")
        by_name[name] = stage
        path = repo_path(stage.get("path"), f"stages[{index}].path", errors)
        row: dict[str, Any] = {"name": name, "path": stage.get("path"), "materialization": stage.get("materialization")}
        if path and path.is_file():
            actual_sha = sha256(path)
            row.update({"exists": True, "sha256": actual_sha, "format": binary_format(path), "size": path.stat().st_size})
            if stage.get("sha256") and actual_sha != stage.get("sha256"):
                errors.append(f"{rel(path)} SHA-256 differs from stages manifest")
            if stage.get("format") and row["format"] != stage.get("format"):
                errors.append(f"{rel(path)} format differs from stages manifest")
            if stage.get("size") and row["size"] != stage.get("size"):
                errors.append(f"{rel(path)} size differs from stages manifest")
        else:
            row["exists"] = False
            if stage.get("name") == "signed_stage0":
                errors.append("signed_stage0 path from stages manifest is missing")
        rows.append(row)
    for stage_name in EXPECTED_CHAIN:
        if stage_name not in by_name:
            errors.append(f"{rel(STAGES_MANIFEST)} missing stage {stage_name}")
    return {"manifest": rel(STAGES_MANIFEST), "stages": rows}


def function_body(text: str, name: str) -> str:
    marker = f"def {name}("
    start = text.find(marker)
    if start < 0:
        return ""
    next_def = text.find("\ndef ", start + len(marker))
    return text[start:] if next_def < 0 else text[start:next_def]


def check_bootstrap_build_contract(errors: list[str]) -> dict[str, Any]:
    text = BOOTSTRAP_REAL.read_text(encoding="utf-8", errors="replace")
    rows: list[dict[str, Any]] = []
    for builder, compiler_name in EXPECTED_STAGE_BUILDERS.items():
        body = function_body(text, builder)
        row = {"builder": builder, "compiler": compiler_name, "uses_entrypoint_build": False, "uses_copy": False}
        if not body:
            errors.append(f"missing bootstrap builder function: {builder}")
            rows.append(row)
            continue
        row["uses_entrypoint_build"] = '"build"' in body and "rel(ENTRYPOINT)" in body and "-o" in body
        row["uses_copy"] = "copy" in body
        if not row["uses_entrypoint_build"]:
            errors.append(f"{builder} must compile {rel(COMPILER_ENTRY)} with the previous stage")
        if row["uses_copy"]:
            errors.append(f"{builder} contains a copy operation; stage builds must compile, not copy")
        rows.append(row)
    return {"script": rel(BOOTSTRAP_REAL), "stage_builders": rows}


def write_reports(payload: dict[str, Any]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# Bootstrap Compile Contract",
        "",
        f"- status: {payload['status']}",
        f"- compiler_modules: {payload['compiler']['module_count']}",
        f"- expected_compiler_modules: {payload['compiler']['expected_module_count']}",
        f"- entrypoint: {payload['compiler']['entrypoint']}",
        f"- stage0_manifest: {payload['stage0'].get('manifest', rel(STAGE0_MANIFEST))}",
        f"- stages_manifest: {payload['stages'].get('manifest', rel(STAGES_MANIFEST))}",
    ]
    if payload["errors"]:
        lines.extend(["", "## Errors"])
        lines.extend(f"- {error}" for error in payload["errors"])
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    errors: list[str] = []
    payload: dict[str, Any] = {
        "schema": "vitte.bootstrap.compile_contract.report.v1",
        "status": "fail",
        "compiler": check_compiler_surface(errors),
        "stage0": check_stage0_manifest(errors),
        "stages": check_stages_manifest(errors),
        "bootstrap_build": check_bootstrap_build_contract(errors),
        "errors": errors,
    }
    payload["status"] = "ok" if not errors else "fail"
    write_reports(payload)
    if errors:
        for error in errors:
            print(f"[bootstrap-compile-contract][error] {error}", file=sys.stderr)
        return 1
    print(
        "[bootstrap-compile-contract] OK "
        f"modules={payload['compiler']['module_count']} "
        f"entrypoint={payload['compiler']['entrypoint']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
