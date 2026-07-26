#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)

cd "$ROOT_DIR"

python3 - <<'PY'
import hashlib
import json
from pathlib import Path

root = Path.cwd()
artifact = root / "toolchain/seed/vittec0.seed"
manifest_path = root / "toolchain/seed/manifest.txt"
policy_path = root / "toolchain/seed/frozen.json"

manifest = {}
for line in manifest_path.read_text(encoding="utf-8").splitlines():
    if line:
        key, _, value = line.partition("=")
        manifest[key] = value
policy = json.loads(policy_path.read_text(encoding="utf-8"))
actual = hashlib.sha256(artifact.read_bytes()).hexdigest()

print("[seed-rotation-report] status=historical-frozen")
print("[seed-rotation-report] usage=bootstrap-fallback-only")
print(f"[seed-rotation-report] artifact=toolchain/seed/vittec0.seed")
print(f"[seed-rotation-report] sha256={actual}")
print(f"[seed-rotation-report] manifest_sha256={manifest.get('sha256', '')}")
print(f"[seed-rotation-report] policy_status={policy.get('status', '')}")
print("[seed-rotation-report] rotation=disabled")
PY
