#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
CONFIG="$ROOT_DIR/toolchain/vitte-bootstrap.toml"

CONFIG_JSON=$(cd "$ROOT_DIR" && python3 tools/bootstrap_config.py --json "$CONFIG")
SEED_REL=$(printf '%s\n' "$CONFIG_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["seed"]["artifact"])')
SOURCE_REL=$(printf '%s\n' "$CONFIG_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["seed"]["source"])')
MANIFEST_REL=$(printf '%s\n' "$CONFIG_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["seed"]["manifest"])')
SEED_SHA=$(printf '%s\n' "$CONFIG_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["seed"]["sha256"])')
OUT_REL=$(printf '%s\n' "$CONFIG_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["compiler"]["stage0"])')
SEED="$ROOT_DIR/$SEED_REL"
SOURCE="$ROOT_DIR/$SOURCE_REL"
MANIFEST="$ROOT_DIR/$MANIFEST_REL"
OUT="$ROOT_DIR/$OUT_REL"
OUT_DIR=${OUT%/*}

if [ ! -f "$SEED" ]; then
    printf '[bootstrap-seed][error] missing seed artifact: %s\n' "$SEED" >&2
    exit 1
fi
if [ ! -x "$SEED" ]; then
    printf '[bootstrap-seed][error] seed artifact is not executable: %s\n' "$SEED" >&2
    exit 1
fi
if [ ! -f "$SOURCE" ]; then
    printf '[bootstrap-seed][error] missing seed source: %s\n' "$SOURCE" >&2
    exit 1
fi
if [ ! -f "$MANIFEST" ]; then
    printf '[bootstrap-seed][error] missing seed manifest: %s\n' "$MANIFEST" >&2
    exit 1
fi

python3 - "$CONFIG_JSON" <<'PY'
import json
import sys
from pathlib import Path

data = json.loads(sys.argv[1])
manifest = {}
for line in Path(data["seed"]["manifest"]).read_text(encoding="utf-8").splitlines():
    if not line:
        continue
    key, sep, value = line.partition("=")
    if not sep:
        raise SystemExit("[bootstrap-seed][error] malformed seed manifest")
    manifest[key] = value
expected = {
    "source_file": data["seed"]["source"],
    "seed_file": data["seed"]["artifact"],
    "sha256": data["seed"]["sha256"],
    "version": data["seed"]["version"],
}
for key, value in expected.items():
    if manifest.get(key) != value:
        raise SystemExit(f"[bootstrap-seed][error] seed manifest mismatch for {key}")
PY

actual_before=$(python3 - "$SEED" <<'PY'
import hashlib
import sys
from pathlib import Path
print(hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest())
PY
)
if [ "$actual_before" != "$SEED_SHA" ]; then
    printf '[bootstrap-seed][error] seed checksum mismatch before copy: expected %s got %s\n' "$SEED_SHA" "$actual_before" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
cp "$SEED" "$OUT"
chmod 755 "$OUT"

actual_after=$(python3 - "$OUT" <<'PY'
import hashlib
import sys
from pathlib import Path
print(hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest())
PY
)
if [ "$actual_after" != "$SEED_SHA" ]; then
    printf '[bootstrap-seed][error] installed seed checksum mismatch after copy: expected %s got %s\n' "$SEED_SHA" "$actual_after" >&2
    exit 1
fi
if [ ! -x "$OUT" ]; then
    printf '[bootstrap-seed][error] installed seed is not executable: %s\n' "$OUT" >&2
    exit 1
fi

cd "$ROOT_DIR"
python3 tools/check_bootstrap_seed_root.py --artifacts

printf '[bootstrap-seed] installed %s\n' "$OUT"
