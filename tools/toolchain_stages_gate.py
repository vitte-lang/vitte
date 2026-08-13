#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "toolchain/bootstrap/stages-manifest.json"
REPORT_JSON = ROOT / "target/reports/toolchain_stages.json"
REPORT_MD = ROOT / "target/reports/toolchain_stages.md"

STAGES: tuple[dict[str, str], ...] = (
    {
        "name": "signed_stage0",
        "kind": "trust-root",
        "path": "toolchain/bootstrap/stage0/macos-arm64/vitte",
        "signature": "toolchain/bootstrap/stage0/macos-arm64/vitte.sig",
        "public_key": "toolchain/bootstrap/stage0/stage0-public.pem",
        "materialized_by": "committed signed artifact",
    },
    {
        "name": "trusted_stage0",
        "kind": "verified-stage0",
        "path": "target/bootstrap-real/stage0/vitte",
        "materialized_by": "make bootstrap-all",
    },
    {
        "name": "bootstrap_compiler",
        "kind": "bootstrap-compiler",
        "path": "target/bootstrap-real/vitte",
        "materialized_by": "make bootstrap-all",
    },
    {
        "name": "stage1",
        "kind": "selfhost-stage",
        "path": "target/stage1/vitte",
        "materialized_by": "make bootstrap-all",
    },
    {
        "name": "stage2",
        "kind": "selfhost-stage",
        "path": "target/stage2/vitte",
        "materialized_by": "make bootstrap-all",
    },
    {
        "name": "release",
        "kind": "release",
        "path": "target/release/vitte",
        "materialized_by": "make bootstrap-all",
    },
    {
        "name": "bin",
        "kind": "installed-release",
        "path": "bin/vitte",
        "materialized_by": "make bootstrap-all",
    },
    {
        "name": "vittec",
        "kind": "installed-release-alias",
        "path": "bin/vittec",
        "materialized_by": "make bootstrap-all",
    },
)

FORBIDDEN_MARKERS = (
    b"vitte-bootstrap-payload",
    b"payload_source",
    b"write_payload_file",
    b"_command_build",
    b"_copy_file",
    b"BOOTSTRAP_FULL_COMPILER",
    b"VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE",
    b"[vitte][error]",
    b"E_CLI_IO: cannot read",
)


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


def stage_row(stage: dict[str, str]) -> dict[str, Any]:
    path = ROOT / stage["path"]
    row: dict[str, Any] = {
        "name": stage["name"],
        "kind": stage["kind"],
        "path": stage["path"],
        "materialized_by": stage["materialized_by"],
        "exists": path.is_file(),
    }
    if "signature" in stage:
        row["signature"] = stage["signature"]
    if "public_key" in stage:
        row["public_key"] = stage["public_key"]
    if not path.is_file():
        return row
    data = path.read_bytes()
    row.update(
        {
            "sha256": sha256(path),
            "size": path.stat().st_size,
            "format": binary_format(path),
            "executable": os.access(path, os.X_OK),
            "forbidden_markers": [
                marker.decode("utf-8", "replace") for marker in FORBIDDEN_MARKERS if marker in data
            ],
        }
    )
    return row


def generate_manifest() -> dict[str, Any]:
    return {
        "schema": "vitte.toolchain.bootstrap.stages.v1",
        "source_of_truth": "src/vitte/compiler/main.vit",
        "canonical_chain": [
            "signed_stage0",
            "trusted_stage0",
            "bootstrap_compiler",
            "stage1",
            "stage2",
            "release",
            "bin",
            "vittec",
        ],
        "parity_policy": {
            "byte_identical": ["signed_stage0", "trusted_stage0", "bootstrap_compiler", "stage1", "stage2", "release", "bin", "vittec"],
            "signature_required": ["signed_stage0"],
            "generated_artifacts": ["trusted_stage0", "bootstrap_compiler", "stage1", "stage2", "release"],
        },
        "stages": [stage_row(stage) for stage in STAGES],
    }


def verify_signature(stage: dict[str, Any], errors: list[str]) -> None:
    openssl = shutil.which("openssl")
    if not openssl:
        errors.append("openssl is required to verify signed_stage0")
        return
    signature = ROOT / str(stage.get("signature", ""))
    public_key = ROOT / str(stage.get("public_key", ""))
    artifact = ROOT / str(stage.get("path", ""))
    if not signature.is_file() or not public_key.is_file() or not artifact.is_file():
        errors.append("signed_stage0 signature inputs are missing")
        return
    completed = subprocess.run(
        [openssl, "dgst", "-sha256", "-verify", str(public_key), "-signature", str(signature), str(artifact)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if completed.returncode != 0 or "Verified OK" not in completed.stdout:
        errors.append(f"signed_stage0 signature verification failed: {completed.stdout.strip()}")


def validate_manifest(manifest: dict[str, Any]) -> tuple[list[str], dict[str, Any]]:
    errors: list[str] = []
    actual = generate_manifest()
    if manifest != actual:
        errors.append("toolchain bootstrap stages manifest is stale")
    stages = manifest.get("stages", [])
    if not isinstance(stages, list):
        return errors + ["manifest stages must be a list"], actual
    by_name = {stage.get("name"): stage for stage in stages if isinstance(stage, dict)}
    required = [stage["name"] for stage in STAGES]
    missing = [name for name in required if name not in by_name]
    if missing:
        errors.append("manifest missing stages: " + ", ".join(missing))
    hashes: dict[str, str] = {}
    for name in required:
        stage = by_name.get(name)
        if not isinstance(stage, dict):
            continue
        if not stage.get("exists"):
            errors.append(f"{name} artifact is missing")
            continue
        if not stage.get("executable"):
            errors.append(f"{name} artifact is not executable")
        if stage.get("format") not in {"Mach-O", "ELF", "PE"}:
            errors.append(f"{name} artifact is not a native binary: {stage.get('format')}")
        markers = stage.get("forbidden_markers", [])
        if markers:
            errors.append(f"{name} contains forbidden markers: {', '.join(markers)}")
        if isinstance(stage.get("sha256"), str):
            hashes[name] = stage["sha256"]
    expected_hashes = {hashes[name] for name in required if name in hashes}
    if len(expected_hashes) != 1:
        errors.append("stage byte parity failed across toolchain manifest")
    signed = by_name.get("signed_stage0")
    if isinstance(signed, dict):
        verify_signature(signed, errors)
    return errors, actual


def write_reports(manifest: dict[str, Any], errors: list[str]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "schema": "vitte.toolchain.bootstrap.stages.report.v1",
        "status": "ok" if not errors else "failed",
        "errors": errors,
        "manifest": manifest,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = ["# Toolchain Bootstrap Stages", "", f"- status: `{payload['status']}`", ""]
    for stage in manifest.get("stages", []):
        lines.append(
            f"- {stage.get('name')}: `{stage.get('path')}` sha=`{stage.get('sha256', 'missing')}`"
        )
    if errors:
        lines.extend(["", "## Errors"])
        lines.extend(f"- {error}" for error in errors)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="verify or update the toolchain bootstrap stages manifest")
    parser.add_argument("--write-manifest", action="store_true")
    args = parser.parse_args(argv)
    actual = generate_manifest()
    if args.write_manifest:
        MANIFEST.parent.mkdir(parents=True, exist_ok=True)
        MANIFEST.write_text(json.dumps(actual, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"[toolchain-stages] wrote {MANIFEST.relative_to(ROOT)}")
        return 0
    if not MANIFEST.is_file():
        write_reports(actual, ["missing toolchain/bootstrap/stages-manifest.json"])
        print("[toolchain-stages][error] missing toolchain/bootstrap/stages-manifest.json", file=sys.stderr)
        return 1
    try:
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        write_reports(actual, [f"manifest is not JSON: {exc}"])
        print(f"[toolchain-stages][error] manifest is not JSON: {exc}", file=sys.stderr)
        return 1
    errors, _actual = validate_manifest(manifest)
    write_reports(manifest, errors)
    if errors:
        print(f"[toolchain-stages][error] failures={len(errors)} report={REPORT_JSON.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[toolchain-stages] ok stages={len(STAGES)} report={REPORT_JSON.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
