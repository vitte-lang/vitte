#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = ROOT / "toolchain/bootstrap/stage0-manifest.json"
SCHEMA = "vitte.bootstrap.stage0.trust.v1"

OS_NAMES = {"Darwin": "macos", "Linux": "linux", "Windows": "windows"}
ARCH_NAMES = {
    "x86_64": "x86_64", "AMD64": "x86_64", "amd64": "x86_64",
    "arm64": "arm64", "aarch64": "arm64", "AARCH64": "arm64",
    "i386": "i386", "i686": "i386", "x86": "i386",
    "riscv64": "riscv64",
}
FORMATS = {"macos": "Mach-O", "linux": "ELF", "windows": "PE"}
FORBIDDEN_MARKERS = (
    b"vitte-bootstrap-payload",
    b"payload_source",
    b"write_payload_file",
    b"BOOTSTRAP_FULL_COMPILER",
    b"VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE",
)


class TrustError(RuntimeError):
    pass


def repo_path(raw: str) -> Path:
    path = (ROOT / raw).resolve()
    try:
        path.relative_to(ROOT)
    except ValueError as exc:
        raise TrustError(f"path escapes repository: {raw}") from exc
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
        b"\xfe\xed\xfa\xce", b"\xfe\xed\xfa\xcf", b"\xce\xfa\xed\xfe", b"\xcf\xfa\xed\xfe",
        b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf",
    }:
        return "Mach-O"
    if data.startswith(b"#!"):
        return "script"
    return "unknown"


def host_tuple() -> tuple[str, str]:
    system = OS_NAMES.get(platform.system(), platform.system().lower())
    machine = ARCH_NAMES.get(platform.machine(), platform.machine().lower())
    return system, machine


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise TrustError(f"cannot read stage0 manifest {path}: {exc}") from exc
    if data.get("schema") != SCHEMA:
        raise TrustError(f"invalid stage0 manifest schema: expected {SCHEMA}")
    if data.get("signature_algorithm") != "openssl-rsa-sha256":
        raise TrustError("stage0 manifest must use openssl-rsa-sha256")
    artifacts = data.get("artifacts")
    if not isinstance(artifacts, list):
        raise TrustError("stage0 manifest artifacts must be a list")
    return data


def select_entry(data: dict[str, Any], os_name: str, arch: str) -> dict[str, str]:
    matches = [
        entry for entry in data["artifacts"]
        if isinstance(entry, dict) and entry.get("os") == os_name and entry.get("arch") == arch
    ]
    if len(matches) != 1:
        raise TrustError(f"expected exactly one signed stage0 for {os_name}-{arch}, found {len(matches)}")
    entry = matches[0]
    required = ("artifact", "sha256", "signature", "public_key", "format")
    missing = [field for field in required if not isinstance(entry.get(field), str) or not entry[field]]
    if missing:
        raise TrustError(f"stage0 entry {os_name}-{arch} is missing: {', '.join(missing)}")
    return entry  # type: ignore[return-value]


def verify_signature(artifact: Path, signature: Path, public_key: Path) -> None:
    openssl = shutil.which("openssl")
    if not openssl:
        raise TrustError("openssl is required to verify the stage0 signature offline")
    completed = subprocess.run(
        [openssl, "dgst", "-sha256", "-verify", str(public_key), "-signature", str(signature), str(artifact)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if completed.returncode != 0 or "Verified OK" not in completed.stdout:
        raise TrustError(f"stage0 signature verification failed: {completed.stdout.strip()}")


def verify_entry(entry: dict[str, str], os_name: str) -> dict[str, object]:
    artifact = repo_path(entry["artifact"])
    signature = repo_path(entry["signature"])
    public_key = repo_path(entry["public_key"])
    for label, path in (("artifact", artifact), ("signature", signature), ("public key", public_key)):
        if not path.is_file():
            raise TrustError(f"missing stage0 {label}: {path.relative_to(ROOT)}")
    actual_sha = sha256(artifact)
    if actual_sha != entry["sha256"]:
        raise TrustError(f"stage0 SHA-256 mismatch: expected {entry['sha256']}, got {actual_sha}")
    actual_format = binary_format(artifact)
    expected_format = FORMATS.get(os_name, entry["format"])
    if entry["format"] != expected_format or actual_format != expected_format:
        raise TrustError(f"stage0 format mismatch: expected {expected_format}, got {actual_format}")
    data = artifact.read_bytes()
    for marker in FORBIDDEN_MARKERS:
        if marker in data:
            raise TrustError(f"stage0 contains forbidden copy/payload marker: {marker.decode()}")
    verify_signature(artifact, signature, public_key)
    return {
        "os": entry["os"], "arch": entry["arch"], "artifact": entry["artifact"],
        "sha256": actual_sha, "format": actual_format, "signature_verified": True,
    }


def verify_manifest(manifest: Path, os_name: str, arch: str) -> dict[str, object]:
    data = load_manifest(manifest)
    entry = select_entry(data, os_name, arch)
    return verify_entry(entry, os_name)


def main(argv: list[str] | None = None) -> int:
    host_os, host_arch = host_tuple()
    parser = argparse.ArgumentParser(description="verify the single signed Vitte stage0 trust root")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--os", default=host_os)
    parser.add_argument("--arch", default=host_arch)
    parser.add_argument("--print-artifact", action="store_true")
    args = parser.parse_args(argv)
    manifest = args.manifest if args.manifest.is_absolute() else ROOT / args.manifest
    try:
        result = verify_manifest(manifest, args.os, args.arch)
    except TrustError as exc:
        print(f"[stage0-trust][error] {exc}", file=sys.stderr)
        return 1
    if args.print_artifact:
        print(result["artifact"])
    else:
        print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
