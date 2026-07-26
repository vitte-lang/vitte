#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
CONFIG="$ROOT_DIR/toolchain/vitte-bootstrap.toml"

cd "$ROOT_DIR"

CONFIG_JSON=$(python3 tools/bootstrap_config.py --json "$CONFIG")
python3 - "$CONFIG_JSON" <<'PY'
import hashlib
import json
import sys
from pathlib import Path

data = json.loads(sys.argv[1])
seed = data["seed"]
source = Path(seed["source"])
artifact = Path(seed["artifact"])
manifest_path = Path(seed["manifest"])

for label, path in (
    ("seed.source", source),
    ("seed.artifact", artifact),
    ("seed.manifest", manifest_path),
):
    if not path.exists():
        raise SystemExit(f"[seed-verify][error] {label} is missing: {path}")
if not source.is_file():
    raise SystemExit(f"[seed-verify][error] seed.source is not a file: {source}")
if not artifact.is_file():
    raise SystemExit(f"[seed-verify][error] seed.artifact is not a file: {artifact}")
if not manifest_path.is_file():
    raise SystemExit(f"[seed-verify][error] seed.manifest is not a file: {manifest_path}")
if hashlib.sha256(artifact.read_bytes()).hexdigest() != seed["sha256"]:
    raise SystemExit("[seed-verify][error] seed.artifact checksum does not match TOML")

manifest = {}
for line in manifest_path.read_text(encoding="utf-8").splitlines():
    if not line:
        continue
    key, sep, value = line.partition("=")
    if not sep:
        raise SystemExit("[seed-verify][error] malformed seed.manifest")
    manifest[key] = value
checks = {
    "source_file": seed["source"],
    "seed_file": seed["artifact"],
    "sha256": seed["sha256"],
    "version": seed["version"],
}
for key, value in checks.items():
    if manifest.get(key) != value:
        raise SystemExit(f"[seed-verify][error] seed.manifest mismatch for {key}")
PY

python3 tools/check_bootstrap_seed_root.py --artifacts
python3 tools/bootstrap_seed_root_test.py
tools/check_seed_contract.sh

printf '[seed-verify] OK\n'
