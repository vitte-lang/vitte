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
    fixture_root = Path(tempfile.mkdtemp(prefix="vitte-stage0-trust-")).resolve()
    original_root = stage0_trust.ROOT
    stage0_trust.ROOT = fixture_root
    try:
        raw = fixture_root
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
                "signature": relative(signature),
                "signature_sha256": hashlib.sha256(signature.read_bytes()).hexdigest(),
                "public_key": relative(public_key),
                "public_key_sha256": hashlib.sha256(public_key.read_bytes()).hexdigest(),
                "format": "ELF",
            }],
        }
        trusted_keys = {("linux", "x86_64"): payload["artifacts"][0]["public_key_sha256"]}
        manifest.write_text(json.dumps(payload), encoding="utf-8")
        verified = stage0_trust.verify_manifest(manifest, "linux", "x86_64", trusted_keys)
        require(verified["signature_verified"] is True, "valid detached signature must pass")

        artifact.write_bytes(artifact.read_bytes() + b"tampered")
        try:
            stage0_trust.verify_manifest(manifest, "linux", "x86_64", trusted_keys)
        except stage0_trust.TrustError as exc:
            require("SHA-256 mismatch" in str(exc), "tampering must fail at the pinned digest")
        else:
            raise AssertionError("tampered stage0 unexpectedly passed")

        payload["artifacts"][0]["sha256"] = hashlib.sha256(artifact.read_bytes()).hexdigest()
        manifest.write_text(json.dumps(payload), encoding="utf-8")
        try:
            stage0_trust.verify_manifest(manifest, "linux", "x86_64", trusted_keys)
        except stage0_trust.TrustError as exc:
            require("signature verification failed" in str(exc), "digest rewrite must not bypass signature")
        else:
            raise AssertionError("stage0 with forged digest unexpectedly passed")

        replacement_private_key = work / "replacement-private.pem"
        replacement_public_key = work / "replacement-public.pem"
        replacement_signature = work / "replacement.sig"
        replacement_artifact = work / "replacement-vitte"
        replacement_artifact.write_bytes(b"\x7fELF\x02\x01\x01\x00replacement-stage0")
        openssl("genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048", "-out", str(replacement_private_key))
        openssl("pkey", "-in", str(replacement_private_key), "-pubout", "-out", str(replacement_public_key))
        openssl("dgst", "-sha256", "-sign", str(replacement_private_key), "-out", str(replacement_signature), str(replacement_artifact))
        entry = payload["artifacts"][0]
        entry.update({
            "artifact": relative(replacement_artifact),
            "sha256": hashlib.sha256(replacement_artifact.read_bytes()).hexdigest(),
            "signature": relative(replacement_signature),
            "signature_sha256": hashlib.sha256(replacement_signature.read_bytes()).hexdigest(),
            "public_key": relative(replacement_public_key),
            "public_key_sha256": hashlib.sha256(replacement_public_key.read_bytes()).hexdigest(),
        })
        manifest.write_text(json.dumps(payload), encoding="utf-8")
        try:
            stage0_trust.verify_manifest(manifest, "linux", "x86_64", trusted_keys)
        except stage0_trust.TrustError as exc:
            require("untrusted stage0 public key" in str(exc), "key substitution must fail at the pinned trust anchor")
        else:
            raise AssertionError("stage0 signed by a substituted key unexpectedly passed")
        print("[stage0-trust-test] ok")
        return 0
    finally:
        stage0_trust.ROOT = original_root
        shutil.rmtree(fixture_root, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
