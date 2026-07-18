#!/usr/bin/env sh
set -eu

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
RUNS="${REPRO_RUNS:-2}"
SEED_BIN="$ROOT_DIR/bin/vittec0"
MANIFEST="$ROOT_DIR/toolchain/seed/manifest.txt"
REPORT_DIR="$ROOT_DIR/target/reports/repro_bootstrap"
REPORT_TXT="$REPORT_DIR/bootstrap_selfhost_repro.txt"
REPORT_JSON="$REPORT_DIR/bootstrap_selfhost_repro.json"

log() { printf '[bootstrap-repro] %s\n' "$1"; }
die() { printf '[bootstrap-repro][error] %s\n' "$1" >&2; exit 1; }

hash_file() {
  file="$1"
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$file" | awk '{print $1}'
  else
    LC_ALL=C shasum -a 256 "$file" | awk '{print $1}'
  fi
}

[ -f "$MANIFEST" ] || die "missing seed manifest: $MANIFEST"

mkdir -p "$REPORT_DIR"
: > "$REPORT_TXT"

run_idx=1
base_seed_hash=""
base_manifest_hash=""
base_version_hash=""

while [ "$run_idx" -le "$RUNS" ]; do
  log "run $run_idx/$RUNS"
  "$ROOT_DIR/scripts/seed/install_seed.sh" >/dev/null
  [ -x "$SEED_BIN" ] || die "seed binary missing after run $run_idx"

  seed_hash=$(hash_file "$SEED_BIN")
  manifest_hash=$(hash_file "$MANIFEST")
  version_file="$REPORT_DIR/run_${run_idx}.version"
  "$SEED_BIN" --version > "$version_file"
  version_hash=$(hash_file "$version_file")

  printf 'run=%s seed=%s manifest=%s version=%s\n' "$run_idx" "$seed_hash" "$manifest_hash" "$version_hash" >> "$REPORT_TXT"

  if [ "$run_idx" -eq 1 ]; then
    base_seed_hash="$seed_hash"
    base_manifest_hash="$manifest_hash"
    base_version_hash="$version_hash"
  else
    [ "$seed_hash" = "$base_seed_hash" ] || die "seed hash drift at run $run_idx"
    [ "$manifest_hash" = "$base_manifest_hash" ] || die "manifest hash drift at run $run_idx"
    [ "$version_hash" = "$base_version_hash" ] || die "version hash drift at run $run_idx"
  fi

  run_idx=$((run_idx + 1))
done

cat > "$REPORT_JSON" <<JSON
{
  "runs": $RUNS,
  "seed_hash": "$base_seed_hash",
  "manifest_hash": "$base_manifest_hash",
  "version_hash": "$base_version_hash",
  "status": "ok"
}
JSON

log "ok report=$REPORT_JSON"
