#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP_ROOT=${TMPDIR:-/tmp}/vitte-scripts-build-repro-$$
VERSION=$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")

cleanup() {
  rm -rf "$TMP_ROOT"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$TMP_ROOT/bin" "$TMP_ROOT/a" "$TMP_ROOT/b"

for command in vitte vittec vittec0; do
  printf '#!/bin/sh\nexit 0\n' > "$TMP_ROOT/bin/$command"
  chmod 0755 "$TMP_ROOT/bin/$command"
done

stage_once() {
  dest=$1
  env \
    VERSION=$VERSION \
    SOURCE_DATE_EPOCH=1700000000 \
    VITTE_BIN_AMD64="$TMP_ROOT/bin/vitte" \
    VITTE_VITTEC_AMD64="$TMP_ROOT/bin/vittec" \
    VITTE_VITTEC0_AMD64="$TMP_ROOT/bin/vittec0" \
    "$ROOT_DIR/scripts_build/stage-installer-payload.sh" "$dest" linux amd64 unix >/dev/null
}

hash_tree() {
  root=$1
  python3 - "$root" <<'PY'
import hashlib
import sys
from pathlib import Path

root = Path(sys.argv[1])
digest = hashlib.sha256()
for path in sorted(p for p in root.rglob("*") if p.is_file()):
    rel = path.relative_to(root).as_posix()
    digest.update(rel.encode())
    digest.update(b"\0")
    digest.update(hashlib.sha256(path.read_bytes()).digest())
print(digest.hexdigest())
PY
}

stage_once "$TMP_ROOT/a"
stage_once "$TMP_ROOT/b"

[ "$(hash_tree "$TMP_ROOT/a")" = "$(hash_tree "$TMP_ROOT/b")" ] || {
  printf '[scripts-build-reproducibility][error] staging output is not reproducible\n' >&2
  exit 1
}

printf '[scripts-build-reproducibility] OK\n'
