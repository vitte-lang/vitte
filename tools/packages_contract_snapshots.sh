#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
PACKAGES_ROOT="${PACKAGES_ROOT:-$ROOT_DIR/src/vitte/packages}"
SNAP_ROOT="${SNAP_ROOT:-$PACKAGES_ROOT/contracts_snapshots}"
UPDATE=0
CRITICAL=(abi core db http)

log() { printf "[packages-contract-snapshots] %s\n" "$*"; }
die() { printf "[packages-contract-snapshots][error] %s\n" "$*" >&2; exit 1; }

usage() {
  cat <<'USAGE'
usage: tools/packages_contract_snapshots.sh [--update]

Checks or updates exports snapshots for critical packages in src/vitte/packages.
USAGE
}

while [ $# -gt 0 ]; do
  case "$1" in
    --update) UPDATE=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown option: $1" ;;
  esac
  shift
done

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$PACKAGES_ROOT" ] || die "missing packages root: $PACKAGES_ROOT"
mkdir -p "$SNAP_ROOT"

tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/vitte-packages-contracts-XXXXXX")"
trap 'rm -rf "$tmp_root"' EXIT

for mod in "${CRITICAL[@]}"; do
  src="$PACKAGES_ROOT/$mod/mod.vit"
  [ -f "$src" ] || die "missing source module: $src"
  mod_snap_dir="$SNAP_ROOT/$mod"
  mkdir -p "$mod_snap_dir"

  snap_all="$mod_snap_dir/$mod.exports"
  snap_public="$mod_snap_dir/$mod.exports.public"
  snap_internal="$mod_snap_dir/$mod.exports.internal"
  snap_hash="$mod_snap_dir/$mod.exports.sha256"

  raw_json="$tmp_root/$mod.module-index.json"
  out="$($BIN check --lang=en --dump-module-index "$src" 2>&1)"
  printf "%s\n" "$out" > "$raw_json"

  gen_all="$tmp_root/$mod.exports"
  gen_public="$tmp_root/$mod.exports.public"
  gen_internal="$tmp_root/$mod.exports.internal"

  python3 - "$mod" "$src" "$raw_json" "$gen_all" "$gen_public" "$gen_internal" <<'PY'
import json
import sys

mod = sys.argv[1]
src_file = sys.argv[2]
raw = open(sys.argv[3], encoding="utf-8").read()
out_all, out_public, out_internal = sys.argv[4], sys.argv[5], sys.argv[6]
start = raw.find("{")
if start < 0:
    raise SystemExit(2)
obj, _ = json.JSONDecoder().raw_decode(raw[start:])
mods = {item.get("key"): item for item in obj.get("modules", [])}
target = None
if "__root__" in mods:
    target = mods["__root__"]
else:
    for item in mods.values():
        if item.get("file") == src_file:
            target = item
            break
if target is None:
    raise SystemExit(3)
exports = sorted(set(target.get("exports", [])))
public = []
internal = []
for sym in exports:
    if sym.startswith("_"):
        internal.append(sym)
    else:
        public.append(sym)
for path, items in ((out_all, exports), (out_public, public), (out_internal, internal)):
    with open(path, "w", encoding="utf-8") as f:
        for item in items:
            f.write(item + "\n")
PY

  sha="$(sha256sum "$gen_all" | awk '{print $1}')"
  printf "%s\n" "$sha" > "$tmp_root/$mod.exports.sha256"

  if [ "$UPDATE" -eq 1 ]; then
    cp "$gen_all" "$snap_all"
    cp "$gen_public" "$snap_public"
    cp "$gen_internal" "$snap_internal"
    cp "$tmp_root/$mod.exports.sha256" "$snap_hash"
    log "updated snapshots for $mod"
    continue
  fi

  [ -f "$snap_all" ] || die "missing snapshot: $snap_all (run --update)"
  [ -f "$snap_public" ] || die "missing snapshot: $snap_public (run --update)"
  [ -f "$snap_internal" ] || die "missing snapshot: $snap_internal (run --update)"
  [ -f "$snap_hash" ] || die "missing snapshot: $snap_hash (run --update)"

  for pair in \
    "$snap_all:$gen_all:all" \
    "$snap_public:$gen_public:public" \
    "$snap_internal:$gen_internal:internal"; do
    IFS=':' read -r expected generated scope <<<"$pair"
    if ! diff -u "$expected" "$generated" >/dev/null 2>&1; then
      diff -u "$expected" "$generated" || true
      die "exports snapshot mismatch for $mod/$scope (run --update if intended)"
    fi
  done

  expected_sha="$(tr -d '\n' < "$snap_hash")"
  if [ "$expected_sha" != "$sha" ]; then
    die "sha256 mismatch for $mod (expected $expected_sha got $sha)"
  fi
done

log "OK"
