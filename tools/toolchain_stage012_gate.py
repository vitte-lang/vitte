#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STAGES = ("stage0", "stage1", "stage2")
ARTIFACTS = {
    "stage0": ROOT / "toolchain/bootstrap/stage0/macos-arm64/vitte",
    "stage1": ROOT / "target/stage1/vitte",
    "stage2": ROOT / "target/stage2/vitte",
}
FORBIDDEN = (b"vitte-bootstrap-payload", b"payload_source", b"BOOTSTRAP_FULL_COMPILER", b"VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE", b"E_CLI_IO: cannot read")
REPORT = ROOT / "target/reports/toolchain_stage012.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def check_contracts(errors: list[str], rows: list[dict[str, object]]) -> None:
    vitte = ROOT / "bin/vitte"
    if not vitte.is_file():
        errors.append("bin/vitte is missing for stage012 syntax checks")
        return
    for stage in STAGES:
        root = ROOT / f"toolchain/{stage}/src"
        files = sorted(root.glob("*.vit"))
        if not files:
            errors.append(f"{stage} has no source contracts")
        for path in files:
            if path.stat().st_size == 0:
                errors.append(f"empty contract: {path.relative_to(ROOT)}")
                continue
            result = subprocess.run([str(vitte), "check", str(path)], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
            if result.returncode != 0:
                errors.append(f"syntax failure: {path.relative_to(ROOT)}: {result.stdout.strip()}")
            rows.append({"stage": stage, "contract": str(path.relative_to(ROOT)), "checked": result.returncode == 0})


def main() -> int:
    errors: list[str] = []
    rows: list[dict[str, object]] = []
    hashes: dict[str, str] = {}
    for stage in STAGES:
        manifest_path = ROOT / f"toolchain/{stage}/{stage}-manifest.json"
        if not manifest_path.is_file():
            errors.append(f"missing manifest: {manifest_path.relative_to(ROOT)}")
            continue
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        artifact = ARTIFACTS[stage]
        if not artifact.is_file():
            errors.append(f"missing artifact: {artifact.relative_to(ROOT)}")
            continue
        digest = sha256(artifact)
        hashes[stage] = digest
        if manifest.get("sha256") != digest or manifest.get("size") != artifact.stat().st_size:
            errors.append(f"stale manifest: {manifest_path.relative_to(ROOT)}")
        if not os.access(artifact, os.X_OK):
            errors.append(f"artifact is not executable: {artifact.relative_to(ROOT)}")
        data = artifact.read_bytes()
        markers = [marker.decode("ascii") for marker in FORBIDDEN if marker in data]
        if markers:
            errors.append(f"forbidden markers in {stage}: {', '.join(markers)}")
        rows.append({"stage": stage, "artifact": str(artifact.relative_to(ROOT)), "sha256": digest, "size": artifact.stat().st_size, "forbidden_markers": markers})
    if len(set(hashes.values())) != 1:
        errors.append("stage0/stage1/stage2 byte parity failed")
    openssl = shutil.which("openssl")
    signature = ROOT / "toolchain/bootstrap/stage0/macos-arm64/vitte.sig"
    public_key = ROOT / "toolchain/bootstrap/stage0/stage0-public.pem"
    if not openssl or not signature.is_file() or not public_key.is_file():
        errors.append("stage0 signature inputs are missing")
    else:
        result = subprocess.run([openssl, "dgst", "-sha256", "-verify", str(public_key), "-signature", str(signature), str(ARTIFACTS["stage0"])], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
        if result.returncode != 0 or "Verified OK" not in result.stdout:
            errors.append(f"stage0 signature verification failed: {result.stdout.strip()}")
    check_contracts(errors, rows)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    payload = {"schema": "vitte.toolchain.stage012.report.v1", "status": "ok" if not errors else "failed", "errors": errors, "rows": rows}
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if errors:
        print(f"[toolchain-stage012][error] failures={len(errors)} report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[toolchain-stage012] ok contracts={sum(1 for row in rows if 'contract' in row)} report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
