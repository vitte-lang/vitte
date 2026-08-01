#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import tempfile
from pathlib import Path

import stage0_trust


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def openssl(*args: str) -> None:
    executable = shutil.which("openssl")
    if not executable:
        raise RuntimeError("openssl is required for stage0 trust tests")
    subprocess.run([executable, *args], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main() -> int:
    fixture_root = stage0_trust.ROOT / "target/bootstrap-real/stage0-trust-test"
    fixture_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=fixture_root) as raw:
        work = Path(raw)
        private_key = work / "private.pem"
        public_key = work / "public.pem"
        artifact = work / "vitte"
        signature = work / "vitte.sig"
        manifest = work / "manifest.json"
        artifact.write_bytes(b"\x7fELF\x02\x01\x01\x00signed-stage0-test")
        openssl("genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048", "-out", str(private_key))
        openssl("pkey", "-in", str(private_key), "-pubout", "-out", str(public_key))
        openssl("dgst", "-sha256", "-sign", str(private_key), "-out", str(signature), str(artifact))
        relative = lambda path: path.relative_to(stage0_trust.ROOT).as_posix()
        payload = {
            "schema": stage0_trust.SCHEMA,
            "signature_algorithm": "openssl-rsa-sha256",
            "artifacts": [{
                "os": "linux", "arch": "x86_64", "artifact": relative(artifact),
                "sha256": hashlib.sha256(artifact.read_bytes()).hexdigest(),
                "signature": relative(signature), "public_key": relative(public_key), "format": "ELF",
            }],
        }
        manifest.write_text(json.dumps(payload), encoding="utf-8")
        verified = stage0_trust.verify_manifest(manifest, "linux", "x86_64")
        require(verified["signature_verified"] is True, "valid detached signature must pass")

        artifact.write_bytes(artifact.read_bytes() + b"tampered")
        try:
            stage0_trust.verify_manifest(manifest, "linux", "x86_64")
        except stage0_trust.TrustError as exc:
            require("SHA-256 mismatch" in str(exc), "tampering must fail at the pinned digest")
        else:
            raise AssertionError("tampered stage0 unexpectedly passed")

        payload["artifacts"][0]["sha256"] = hashlib.sha256(artifact.read_bytes()).hexdigest()
        manifest.write_text(json.dumps(payload), encoding="utf-8")
        try:
            stage0_trust.verify_manifest(manifest, "linux", "x86_64")
        except stage0_trust.TrustError as exc:
            require("signature verification failed" in str(exc), "digest rewrite must not bypass signature")
        else:
            raise AssertionError("stage0 with forged digest unexpectedly passed")
    print("[stage0-trust-test] ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
