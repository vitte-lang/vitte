#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
CACHE_DIR="${CACHE_DIR:-$ROOT_DIR/.vitte-cache}"
STAMP_FILE="${STAMP_FILE:-$CACHE_DIR/ci-fast-compiler.sha256}"

log() { printf "[ci-fast-compiler] %s\n" "$*"; }
die() { printf "[ci-fast-compiler][error] %s\n" "$*" >&2; exit 1; }

mkdir -p "$CACHE_DIR"

compute_hash() {
  python3 - "$ROOT_DIR" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
paths = []
paths.extend(sorted((root / "tests" / "diag_snapshots" / "resolve").rglob("*")))
paths.extend(sorted((root / "tests" / "modules" / "snapshots").rglob("*")))
paths.append(root / "tests" / "vitte_packages_runtime_matrix.vit")
paths.extend(sorted((root / "tests" / "explain_snapshots").rglob("*")))
paths.extend([
    root / "docs" / "book" / "scripts" / "sync_grammar_surface.py",
    root / "src" / "vitte" / "grammar" / "vitte.ebnf",
    root / "docs" / "book" / "grammar-surface.ebnf",
    root / "tools" / "diag_snapshots.sh",
    root / "tools" / "modules_snapshots.sh",
    root / "tools" / "explain_snapshots.sh",
    root / "tools" / "check_module_shape_policy.py",
])

h = hashlib.sha256()
for p in paths:
    if not p.exists() or p.is_dir():
        continue
    rel = p.relative_to(root).as_posix()
    data = p.read_bytes()
    h.update(rel.encode("utf-8"))
    h.update(b"\0")
    h.update(hashlib.sha256(data).hexdigest().encode("utf-8"))
    h.update(b"\n")
print(h.hexdigest())
PY
}

new_hash="$(compute_hash)"
old_hash=""
if [[ -f "$STAMP_FILE" ]]; then
  old_hash="$(tr -d '\n' < "$STAMP_FILE")"
fi

if [[ -n "$old_hash" && "$old_hash" == "$new_hash" ]]; then
  log "cache hit (grammar/resolve snapshots/module snapshots/runtime matrix unchanged)"
  exit 0
fi

log "cache miss, running compiler-fast checks"
[[ -x "$ROOT_DIR/bin/vitte" ]] || die "missing binary: $ROOT_DIR/bin/vitte (run make build first)"

make -C "$ROOT_DIR" grammar-check
make -C "$ROOT_DIR" module-shape-policy
make -C "$ROOT_DIR" resolve-tests
make -C "$ROOT_DIR" modules-snapshots
make -C "$ROOT_DIR" explain-snapshots
make -C "$ROOT_DIR" runtime-matrix-modules

printf "%s\n" "$new_hash" > "$STAMP_FILE"
log "updated cache stamp: $STAMP_FILE"
