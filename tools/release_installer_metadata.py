#!/usr/bin/env python3
"""Create fail-closed installer manifests, SBOMs, signatures and provenance."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import subprocess
import time
from pathlib import Path


METADATA_NAMES = {
    "ATTESTATION.json",
    "CHECKSUMS.txt",
    "INSTALLERS.json",
    "SIGNATURES.json",
    "SBOM.spdx.json",
    "SBOM.cyclonedx.json",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def is_artifact(path: Path) -> bool:
    name = path.name
    if not path.is_file() or name in METADATA_NAMES:
        return False
    if name.endswith((".sha256", ".MANIFEST.json", ".signature.json", ".sig")):
        return False
    return bool(
        re.search(r"\.(deb|pkg|dmg|exe)$", name)
        or name.endswith((".tar.gz", ".tar.xz"))
    )


def classify(name: str) -> tuple[str, str]:
    patterns = (
        (r"vitte_[^_]+_([^.]+)\.deb$", "linux"),
        (r"vitte-[^-]+-portable-([^-]+)-([^.]+)\.tar\.gz$", None),
        (r"vitte-[^-]+-freebsd-([^-]+)\.pkg$", "freebsd"),
        (r"vitte-[^-]+-macos-(.+?)\.(?:pkg|dmg)$", "macos"),
        (r"vitte-[^-]+-solaris-([^-]+)(?:-spool)?\.(?:pkg|tar\.gz)$", "solaris"),
        (r"vitte-[^-]+-windows-([^-]+)-(?:installer\.exe|nsis\.tar\.gz)$", "windows"),
    )
    for expression, fixed_os in patterns:
        match = re.fullmatch(expression, name)
        if match:
            if fixed_os is None:
                return match.group(1), match.group(2)
            return fixed_os, match.group(1)
    bsd = re.fullmatch(r"vitte-[^-]+-([^-]+)-[^-]+-([^-]+)-installer\.tar\.xz", name)
    return (bsd.group(1), bsd.group(2)) if bsd else ("unknown", "unknown")


def git_value(root: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(root), *args], text=True,
        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
    )
    return result.stdout.strip() if result.returncode == 0 else "unknown"


def verify_signature(artifact: Path, public_key: Path | None) -> dict[str, object]:
    signature = artifact.with_name(artifact.name + ".sig")
    row: dict[str, object] = {
        "name": artifact.name,
        "sha256": sha256(artifact),
        "signed": False,
        "verified": False,
        "signature": None,
        "verification": "missing detached signature",
    }
    if not signature.is_file():
        return row
    row["signature"] = signature.name
    row["signature_sha256"] = sha256(signature)
    if public_key is None:
        row["verification"] = "detached signature present; VITTE_RELEASE_PUBLIC_KEY not set"
        return row
    result = subprocess.run(
        ["openssl", "dgst", "-sha256", "-verify", str(public_key), "-signature", str(signature), str(artifact)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    row["signed"] = result.returncode == 0
    row["verified"] = result.returncode == 0
    row["verification"] = result.stdout.strip()
    return row


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--version", required=True)
    parser.add_argument("--source-date-epoch", required=True, type=int)
    parser.add_argument("--sbom", action="store_true")
    parser.add_argument("--require-signatures", action="store_true")
    parser.add_argument("--public-key", type=Path)
    args = parser.parse_args()

    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    root = Path(__file__).resolve().parents[1]
    artifacts = []
    for path in sorted(item for item in out.iterdir() if is_artifact(item)):
        os_name, arch = classify(path.name)
        item = {
            "name": path.name,
            "size": path.stat().st_size,
            "sha256": sha256(path),
            "os": os_name,
            "arch": arch,
        }
        artifacts.append(item)
        manifest = {
            "schema": "org.vitte.installer-artifact.v2",
            "version": args.version,
            **item,
            "abi": f"{os_name}-{arch}",
            "libc": {
                "linux": "glibc-or-musl", "solaris": "solaris-libc",
                "macos": "libSystem", "windows": "msvcrt-compatible",
            }.get(os_name, "bsd-libc" if "bsd" in os_name else "unknown"),
            "minimum_version": "platform-matrix",
            "installed_commands": ["vitte", "vittec", "vitte-installer-doctor"],
            "contents": ["compiler", "runtime", "stdlib", "sources", "documentation"],
        }
        (out / f"{path.name}.MANIFEST.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )

    checksums = "".join(f"{item['sha256']}  {item['name']}\n" for item in artifacts)
    (out / "CHECKSUMS.txt").write_text(checksums, encoding="utf-8")
    (out / "INSTALLERS.json").write_text(json.dumps({
        "schema": "org.vitte.installers.v2", "version": args.version,
        "source_date_epoch": args.source_date_epoch,
        "artifact_count": len(artifacts), "artifacts": artifacts,
    }, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    public_key = args.public_key.resolve() if args.public_key else None
    signatures = [verify_signature(out / item["name"], public_key) for item in artifacts]
    signature_report = {
        "schema": "org.vitte.installer-signatures.v2",
        "version": args.version,
        "verification_key": str(public_key) if public_key else None,
        "all_verified": bool(signatures) and all(row["verified"] for row in signatures),
        "artifacts": signatures,
    }
    (out / "SIGNATURES.json").write_text(
        json.dumps(signature_report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    if args.sbom:
        namespace_seed = hashlib.sha256(checksums.encode()).hexdigest()
        spdx = {
            "spdxVersion": "SPDX-2.3", "dataLicense": "CC0-1.0",
            "SPDXID": "SPDXRef-DOCUMENT", "name": f"Vitte installers {args.version}",
            "documentNamespace": f"https://vitte-lang.org/sbom/{args.version}/{namespace_seed}",
            "creationInfo": {"created": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(args.source_date_epoch)), "creators": ["Tool: vitte-release-installer-metadata"]},
            "packages": [{
                "SPDXID": "SPDXRef-" + re.sub(r"[^A-Za-z0-9.-]", "-", item["name"]),
                "name": item["name"], "versionInfo": args.version,
                "downloadLocation": "NOASSERTION", "filesAnalyzed": False,
                "checksums": [{"algorithm": "SHA256", "checksumValue": item["sha256"]}],
            } for item in artifacts],
        }
        cyclonedx = {
            "bomFormat": "CycloneDX", "specVersion": "1.5", "serialNumber": f"urn:uuid:{namespace_seed[:8]}-{namespace_seed[8:12]}-{namespace_seed[12:16]}-{namespace_seed[16:20]}-{namespace_seed[20:32]}", "version": 1,
            "metadata": {"timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(args.source_date_epoch)), "component": {"type": "application", "name": "vitte-installers", "version": args.version}},
            "components": [{"type": "file", "name": item["name"], "version": args.version, "hashes": [{"alg": "SHA-256", "content": item["sha256"]}]} for item in artifacts],
        }
        (out / "SBOM.spdx.json").write_text(json.dumps(spdx, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        (out / "SBOM.cyclonedx.json").write_text(json.dumps(cyclonedx, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    commit = git_value(root, "rev-parse", "HEAD")
    dirty = bool(git_value(root, "status", "--porcelain"))
    attestation = {
        "_type": "https://in-toto.io/Statement/v1",
        "subject": [{"name": item["name"], "digest": {"sha256": item["sha256"]}} for item in artifacts],
        "predicateType": "https://slsa.dev/provenance/v1",
        "predicate": {
            "buildDefinition": {
                "buildType": "https://vitte-lang.org/buildtypes/installers/v1",
                "externalParameters": {"version": args.version, "source_date_epoch": args.source_date_epoch},
                "resolvedDependencies": [{"uri": "git+file://vitte", "digest": {"gitCommit": commit}}],
            },
            "runDetails": {
                "builder": {"id": "tools/release_installer_metadata.py"},
                "metadata": {"invocationId": namespace_seed if args.sbom else hashlib.sha256(checksums.encode()).hexdigest(), "sourceDirty": dirty, "host": platform.platform()},
            },
        },
    }
    (out / "ATTESTATION.json").write_text(json.dumps(attestation, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if args.require_signatures and not signature_report["all_verified"]:
        missing = [row["name"] for row in signatures if not row["verified"]]
        raise SystemExit("unverified installer signatures: " + ", ".join(missing))
    print(f"[installer-metadata] artifacts={len(artifacts)} signatures={sum(bool(row['verified']) for row in signatures)} sbom={int(args.sbom)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
