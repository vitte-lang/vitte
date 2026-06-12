#!/usr/bin/env sh
set -eu

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
RUNS="${REPRO_RUNS:-2}"
ENTRY_SRC="${REPRO_SRC:-$ROOT_DIR/toolchain/stage2/src/main.vit}"
STAGE2_SCRIPT="$ROOT_DIR/toolchain/scripts/bootstrap/stage2.sh"
STAGE2_BIN="$ROOT_DIR/bin/vittec"
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

[ -x "$STAGE2_SCRIPT" ] || die "missing stage2 bootstrap script: $STAGE2_SCRIPT"
[ -f "$ENTRY_SRC" ] || die "missing source for IR hash: $ENTRY_SRC"

mkdir -p "$REPORT_DIR"
: > "$REPORT_TXT"

run_idx=1
base_bin_hash=""
base_ir_hash=""
base_source_hash=""
base_version_hash=""

while [ "$run_idx" -le "$RUNS" ]; do
  log "run $run_idx/$RUNS"

  VITTE_SELF_CHECK=0 "$STAGE2_SCRIPT" >/dev/null

  [ -x "$STAGE2_BIN" ] || die "stage2 binary missing after run $run_idx"

  bin_hash=$(hash_file "$STAGE2_BIN")
  ir_file="$REPORT_DIR/run_${run_idx}.ir"
  if "$STAGE2_BIN" dump-native-ir --src "$ENTRY_SRC" --ir-version v2 > "$ir_file" 2> "$REPORT_DIR/run_${run_idx}.ir.err"; then
    :
  else
    die "dump-native-ir failed on run $run_idx"
  fi
  ir_hash=$(hash_file "$ir_file")

  source_file="$REPORT_DIR/run_${run_idx}.selfhost_source"
  "$STAGE2_BIN" selfhost-source > "$source_file"
  source_hash=$(hash_file "$source_file")

  version_file="$REPORT_DIR/run_${run_idx}.version"
  "$STAGE2_BIN" --version > "$version_file"
  version_hash=$(hash_file "$version_file")

  printf 'run=%s bin=%s ir=%s source=%s version=%s\n' "$run_idx" "$bin_hash" "$ir_hash" "$source_hash" "$version_hash" >> "$REPORT_TXT"

  if [ "$run_idx" -eq 1 ]; then
    base_bin_hash="$bin_hash"
    base_ir_hash="$ir_hash"
    base_source_hash="$source_hash"
    base_version_hash="$version_hash"
  else
    [ "$bin_hash" = "$base_bin_hash" ] || die "binary hash drift at run $run_idx"
    [ "$ir_hash" = "$base_ir_hash" ] || die "IR hash drift at run $run_idx"
    [ "$source_hash" = "$base_source_hash" ] || die "selfhost-source hash drift at run $run_idx"
    [ "$version_hash" = "$base_version_hash" ] || die "version hash drift at run $run_idx"
  fi

  run_idx=$((run_idx + 1))
done

cat > "$REPORT_JSON" <<JSON
{
  "runs": $RUNS,
  "bin_hash": "$base_bin_hash",
  "ir_hash": "$base_ir_hash",
  "selfhost_source_hash": "$base_source_hash",
  "version_hash": "$base_version_hash",
  "status": "ok"
}
JSON

log "ok report=$REPORT_JSON"
