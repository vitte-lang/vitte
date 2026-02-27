#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
BASE_DIR="$ROOT_DIR/tests/modules/contracts"
CRITICAL_MODULES=(abi http db core actor)
UPDATE=0
ALLOW_BREAKING=0
CURRENT_VERSION="${CURRENT_VERSION:-}"
BASELINE_VERSION="${BASELINE_VERSION:-}"

log() { printf "[modules-contract-snapshots] %s\n" "$*"; }
die() { printf "[modules-contract-snapshots][error] %s\n" "$*" >&2; exit 1; }

usage() {
  cat <<'USAGE'
usage: tools/modules_contract_snapshots.sh [--update] [--allow-breaking] [--current-version X] [--baseline-version X]

Options:
  --update                Regenerate <module>.exports(.public/.internal) + .sha256 snapshots.
  --allow-breaking        Bypass "breaking only in major" guard.
  --current-version <v>   Current release version (default: ./version).
  --baseline-version <v>  Baseline contract version (default: tests/modules/contracts/BASELINE_VERSION).
USAGE
}

while [ $# -gt 0 ]; do
  case "$1" in
    --update) UPDATE=1 ;;
    --allow-breaking) ALLOW_BREAKING=1 ;;
    --current-version)
      shift
      CURRENT_VERSION="${1:-}"
      [ -n "$CURRENT_VERSION" ] || die "--current-version requires a value"
      ;;
    --baseline-version)
      shift
      BASELINE_VERSION="${1:-}"
      [ -n "$BASELINE_VERSION" ] || die "--baseline-version requires a value"
      ;;
    --current-version=*) CURRENT_VERSION="${1#--current-version=}" ;;
    --baseline-version=*) BASELINE_VERSION="${1#--baseline-version=}" ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown option: $1"
      ;;
  esac
  shift
done

if [ -z "$CURRENT_VERSION" ] && [ -f "$ROOT_DIR/version" ]; then
  CURRENT_VERSION="$(tr -d '\n' < "$ROOT_DIR/version")"
fi
if [ -z "$BASELINE_VERSION" ] && [ -f "$BASE_DIR/BASELINE_VERSION" ]; then
  BASELINE_VERSION="$(tr -d '\n' < "$BASE_DIR/BASELINE_VERSION")"
fi
[ -n "$CURRENT_VERSION" ] && CURRENT_VERSION="$(sed -E 's/.*([0-9]+\.[0-9]+\.[0-9]+).*/\1/' <<<"$CURRENT_VERSION")"
[ -n "$BASELINE_VERSION" ] && BASELINE_VERSION="$(sed -E 's/.*([0-9]+\.[0-9]+\.[0-9]+).*/\1/' <<<"$BASELINE_VERSION")"
[ -n "$CURRENT_VERSION" ] || CURRENT_VERSION="0.0.0"
[ -n "$BASELINE_VERSION" ] || BASELINE_VERSION="$CURRENT_VERSION"

current_major="${CURRENT_VERSION%%.*}"
baseline_major="${BASELINE_VERSION%%.*}"

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$BASE_DIR" ] || die "missing contracts dir: $BASE_DIR"

tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/vitte-contracts-XXXXXX")"
trap 'rm -rf "$tmp_root"' EXIT

for mod in "${CRITICAL_MODULES[@]}"; do
  fixture="$BASE_DIR/$mod/main.vit"
  exports_all="$BASE_DIR/$mod/$mod.exports"
  exports_public="$BASE_DIR/$mod/$mod.exports.public"
  exports_internal="$BASE_DIR/$mod/$mod.exports.internal"
  exports_hash="$BASE_DIR/$mod/$mod.exports.sha256"
  index_must="$BASE_DIR/$mod/$mod.index.must"

  [ -f "$fixture" ] || die "missing fixture: $fixture"
  [ -f "$index_must" ] || die "missing index snapshot: $index_must"
  if [ "$UPDATE" -eq 0 ]; then
    [ -f "$exports_all" ] || die "missing exports snapshot: $exports_all"
    [ -f "$exports_public" ] || die "missing exports snapshot: $exports_public"
    [ -f "$exports_internal" ] || die "missing exports snapshot: $exports_internal"
    [ -f "$exports_hash" ] || die "missing hash snapshot: $exports_hash"
  fi

  log "$mod (current=$CURRENT_VERSION, baseline=$BASELINE_VERSION)"
  out="$($BIN check --lang=en --dump-module-index "$fixture" 2>&1)"
  grep -Fq "[driver] mir ok" <<<"$out" || die "fixture failed: $fixture"

  while IFS= read -r needle; do
    [[ -z "$needle" ]] && continue
    grep -Fq "$needle" <<<"$out" || {
      printf "%s\n" "$out"
      die "missing index snapshot needle '$needle' for module '$mod'"
    }
  done < "$index_must"

  raw_json="$tmp_root/$mod.out.txt"
  printf "%s\n" "$out" > "$raw_json"

  gen_all="$tmp_root/$mod.exports"
  gen_public="$tmp_root/$mod.exports.public"
  gen_internal="$tmp_root/$mod.exports.internal"
  python3 - "$mod" "$raw_json" "$gen_all" "$gen_public" "$gen_internal" <<'PY'
import json
import sys

mod = sys.argv[1]
raw_path = sys.argv[2]
out_all = sys.argv[3]
out_public = sys.argv[4]
out_internal = sys.argv[5]
text = open(raw_path, encoding="utf-8").read()
start = text.find("{")
if start < 0:
    raise SystemExit("missing json block")
obj, _ = json.JSONDecoder().raw_decode(text[start:])
mods = {item.get("key"): item for item in obj.get("modules", [])}
key = f"vitte/{mod}"
exports = sorted(mods.get(key, {}).get("exports", []))

public = []
internal = []
for sym in exports:
    lower = sym.lower()
    if sym.startswith("_") or lower.startswith("internal_") or "internal" in lower:
        internal.append(sym)
    else:
        public.append(sym)

for path, items in ((out_all, exports), (out_public, public), (out_internal, internal)):
    with open(path, "w", encoding="utf-8") as f:
        for sym in items:
            f.write(sym + "\n")
PY
  gen_hash="$(sha256sum "$gen_all" | awk '{print $1}')"

  if [ "$UPDATE" -eq 1 ]; then
    cp "$gen_all" "$exports_all"
    cp "$gen_public" "$exports_public"
    cp "$gen_internal" "$exports_internal"
    printf "%s\n" "$gen_hash" > "$exports_hash"
    log "updated snapshots for $mod"
    continue
  fi

  old_hash="$(tr -d '\n' < "$exports_hash")"
  if [ "$old_hash" != "$gen_hash" ]; then
    log "hash drift for $mod: $old_hash -> $gen_hash"
  fi

  for pair in \
    "$exports_all:$gen_all:all" \
    "$exports_public:$gen_public:public" \
    "$exports_internal:$gen_internal:internal"; do
    old="${pair%%:*}"
    rest="${pair#*:}"
    new="${rest%%:*}"
    scope="${pair##*:}"
    if diff -u "$old" "$new" >/dev/null 2>&1; then
      continue
    fi

    log "contract diff ($scope) for $mod"
    added="$(comm -13 "$old" "$new" || true)"
    removed="$(comm -23 "$old" "$new" || true)"
    if [ -n "$added" ]; then
      printf "[modules-contract-snapshots] added (%s/%s):\n%s\n" "$mod" "$scope" "$added"
    fi
    if [ -n "$removed" ]; then
      printf "[modules-contract-snapshots] removed (%s/%s):\n%s\n" "$mod" "$scope" "$removed"
      if [ "$current_major" = "$baseline_major" ] && [ "$ALLOW_BREAKING" -ne 1 ]; then
        die "breaking export removal in '$mod' requires major bump (baseline=$BASELINE_VERSION current=$CURRENT_VERSION)"
      fi
    fi
    diff -u "$old" "$new" || true
    die "exports snapshot mismatch for module '$mod' scope '$scope' (run with --update if intended)"
  done

  if [ "$old_hash" != "$gen_hash" ]; then
    die "hash mismatch for $mod (expected $old_hash, got $gen_hash). run --update after review"
  fi
done

if [ "$UPDATE" -eq 1 ]; then
  printf "%s\n" "$CURRENT_VERSION" > "$BASE_DIR/BASELINE_VERSION"
fi

log "OK"
