#!/usr/bin/env python3
"""Last fail-closed gate for publishing Vitte 0.1.0."""
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VERSION = os.environ.get("VITTE_RELEASE_VERSION", "0.1.0")
ARCHIVE = Path(os.environ.get("FINAL_ARCHIVE", ROOT / "dist" / f"vitte-{VERSION}.tar.xz")).resolve()
PUBLIC_KEY = Path(os.environ.get("VITTE_RELEASE_PUBLIC_KEY", "")).resolve() if os.environ.get("VITTE_RELEASE_PUBLIC_KEY") else None
REPRO_A = Path(os.environ.get("REPRO_ATTESTATION_A", ROOT / "target/reproducibility/machine-a.json")).resolve()
REPRO_B = Path(os.environ.get("REPRO_ATTESTATION_B", ROOT / "target/reproducibility/machine-b.json")).resolve()
REPORT = ROOT / "target/reports/final_release_gate.json"


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def load_json(path: Path, failures: list[str]) -> dict[str, object]:
    if not path.is_file():
        failures.append(f"missing {path}")
        return {}
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        failures.append(f"invalid JSON {path}: {exc}")
        return {}
    return value if isinstance(value, dict) else {}


def main() -> int:
    failures: list[str] = []
    status = subprocess.run(
        ["git", "status", "--porcelain", "--untracked-files=all"], cwd=ROOT,
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    dirty = [line for line in status.stdout.splitlines() if line.strip()]
    if status.returncode != 0 or dirty:
        failures.append("checkout is not clean: " + ", ".join(dirty[:20]))

    real = load_json(ROOT / "target/reports/real_release_gate.json", failures)
    if real.get("status") not in {"PASS", "pass", "ok"} or real.get("failures"):
        failures.append("real-release-gate is not at zero failures")

    release = ROOT / "target/release/vitte"
    if not release.is_file():
        failures.append("missing target/release/vitte")
        release_sha = ""
    else:
        release_sha = digest(release)

    reproductions = [load_json(REPRO_A, failures), load_json(REPRO_B, failures)]
    machine_ids = {str(item.get("machine_id", "")) for item in reproductions}
    if "" in machine_ids or len(machine_ids) != 2:
        failures.append("reproducibility evidence must name two distinct machines")
    for path, item in zip((REPRO_A, REPRO_B), reproductions):
        if item.get("release_sha256") != release_sha or item.get("stage1_stage2_parity") is not True:
            failures.append(f"reproducibility mismatch in {path}")

    required = [
        ARCHIVE, ARCHIVE.with_name(ARCHIVE.name + ".sha256"), ARCHIVE.with_name(ARCHIVE.name + ".sig"),
        ROOT / "pkgout/SBOM.spdx.json", ROOT / "pkgout/SBOM.cyclonedx.json",
        ROOT / "pkgout/ATTESTATION.json", ROOT / "docs/release/compatibility_matrix.md",
        ROOT / "CHANGELOG.md",
    ]
    for path in required:
        if not path.is_file() or path.stat().st_size == 0:
            failures.append(f"missing final release evidence {path}")

    if ARCHIVE.is_file() and ARCHIVE.with_name(ARCHIVE.name + ".sha256").is_file():
        expected = ARCHIVE.with_name(ARCHIVE.name + ".sha256").read_text(encoding="utf-8").split()[0]
        if expected != digest(ARCHIVE):
            failures.append("final archive checksum mismatch")
    if PUBLIC_KEY is None or not PUBLIC_KEY.is_file():
        failures.append("VITTE_RELEASE_PUBLIC_KEY must name the release PEM public key")
    elif ARCHIVE.is_file() and ARCHIVE.with_name(ARCHIVE.name + ".sig").is_file():
        verified = subprocess.run([
            "openssl", "dgst", "-sha256", "-verify", str(PUBLIC_KEY),
            "-signature", str(ARCHIVE.with_name(ARCHIVE.name + ".sig")), str(ARCHIVE),
        ], cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if verified.returncode != 0:
            failures.append("final archive signature verification failed")

    payload = {
        "schema": "org.vitte.final-release-gate.v1", "version": VERSION,
        "status": "PASS" if not failures else "FAIL", "archive": str(ARCHIVE),
        "release_sha256": release_sha, "reproduction_attestations": [str(REPRO_A), str(REPRO_B)],
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[final-release-gate][error] {failure}", file=sys.stderr)
        print(f"[final-release-gate] report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[final-release-gate] PASS version={VERSION} report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
