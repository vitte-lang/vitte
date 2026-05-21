#!/usr/bin/env sh
set -eu

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
RUNS="${REPRO_RUNS:-2}"
ENTRY_SRC="${REPRO_SRC:-$ROOT_DIR/src/vitte/compiler/driver/compiler.vit}"
STAGE2_SCRIPT="$ROOT_DIR/toolchain/scripts/bootstrap/stage2.sh"
STAGE2_BIN="$ROOT_DIR/bin/vittec"
REPORT_DIR="$ROOT_DIR/target/reports/repro_bootstrap"
REPORT_TXT="$REPORT_DIR/bootstrap_selfhost_repro.txt"
REPORT_JSON="$REPORT_DIR/bootstrap_selfhost_repro.json"

log() { printf '[bootstrap-repro] %s\n' "$1"; }
die() { printf '[bootstrap-repro][error] %s\n' "$1" >&2; exit 1; }

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

  bin_hash=$(sha256sum "$STAGE2_BIN" | awk '{print $1}')
  ir_file="$REPORT_DIR/run_${run_idx}.ir"
  if "$STAGE2_BIN" dump-native-ir --src "$ENTRY_SRC" --ir-version v2 > "$ir_file" 2> "$REPORT_DIR/run_${run_idx}.ir.err"; then
    :
  else
    die "dump-native-ir failed on run $run_idx"
  fi
  ir_hash=$(sha256sum "$ir_file" | awk '{print $1}')

  source_file="$REPORT_DIR/run_${run_idx}.selfhost_source"
  "$STAGE2_BIN" selfhost-source > "$source_file"
  source_hash=$(sha256sum "$source_file" | awk '{print $1}')

  version_file="$REPORT_DIR/run_${run_idx}.version"
  "$STAGE2_BIN" --version > "$version_file"
  version_hash=$(sha256sum "$version_file" | awk '{print $1}')

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
