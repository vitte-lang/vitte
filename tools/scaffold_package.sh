#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<USAGE
usage: $0 <package-name> [--diag-prefix=VITTE-X] [--strict-facade] [--with-doctor] [--with-quickfix]
USAGE
}

if [ $# -lt 1 ]; then
  usage >&2
  exit 1
fi

PKG="$1"
shift || true
DIAG_PREFIX="VITTE-X"
STRICT_FACADE=0
WITH_DOCTOR=0
WITH_QUICKFIX=0
for arg in "$@"; do
  case "$arg" in
    --diag-prefix=*) DIAG_PREFIX="${arg#*=}" ;;
    --strict-facade) STRICT_FACADE=1 ;;
    --with-doctor) WITH_DOCTOR=1 ;;
    --with-quickfix) WITH_QUICKFIX=1 ;;
    *) echo "[scaffold-package][error] unknown arg: $arg" >&2; usage >&2; exit 1 ;;
  esac
done

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
PKG_DIR="$ROOT_DIR/src/vitte/packages/$PKG"
CONTRACT_DIR="$ROOT_DIR/tests/modules/contracts/$PKG"

mkdir -p "$PKG_DIR/internal" "$CONTRACT_DIR" "$ROOT_DIR/tests/$PKG/table" "$ROOT_DIR/tests/$PKG/fuzz_seeds" "$ROOT_DIR/tests/$PKG/bench" "$ROOT_DIR/docs/$PKG"

cat > "$PKG_DIR/mod.vit" <<EOF2
<<<
mod.vit
package vitte/$PKG
>>>
# PREAMBLE (API stable facade)
# - public facade only; heavy logic must stay in internal/*
# - no import-time side effects

space vitte/$PKG

use vitte/$PKG/internal/runtime as ${PKG}_runtime_pkg

pick ${PKG^}Error {
  Failure(code: string, message: string, context: string)
}

pick ${PKG^}Result[T] {
  Ok(value: T)
  Err(error: ${PKG^}Error)
}

proc diagnostics_message(code: string) -> string {
  give "${PKG} diagnostic " + code
}

proc diagnostics_quickfix(code: string) -> string {
  give "review ${PKG} usage for " + code
}

proc diagnostics_doc_url(code: string) -> string {
  give "https://docs.vitte.dev/diagnostics/" + code
}

proc api_version() -> string {
  give "v1"
}
EOF2

if [ "$WITH_DOCTOR" -eq 1 ] || [ "$STRICT_FACADE" -eq 1 ]; then
  cat >> "$PKG_DIR/mod.vit" <<'EOF2'

proc doctor_status() -> string {
  give "vitte/PKG doctor:ready api=" + api_version()
}
EOF2
  sed -i "s/vitte\/PKG/vitte\/$PKG/g" "$PKG_DIR/mod.vit"
fi

if [ "$WITH_QUICKFIX" -eq 1 ] || [ "$STRICT_FACADE" -eq 1 ]; then
  cat >> "$PKG_DIR/mod.vit" <<'EOF2'

proc quickfix_payload_schema_version() -> string {
  give "v1"
}

proc quickfix_preview(payload: string) -> string {
  if payload.len == 0 { give "quickfix: no-op" }
  give "schema=" + quickfix_payload_schema_version() + " " + diagnostics_quickfix(payload)
}

proc quickfix_apply(payload: string) -> string {
  if payload.len == 0 { give "quickfix apply: no-op" }
  give "schema=" + quickfix_payload_schema_version() + " quickfix apply: " + payload
}
EOF2
fi

cat >> "$PKG_DIR/mod.vit" <<EOF2

proc ready() -> bool {
  give true
}

proc package_meta() -> string {
  give "vitte/$PKG"
}

<<< ROLE-CONTRACT
package: vitte/$PKG
owner: @vitte/$PKG
stability: stable
since: 1.0.0
deprecated_in: -
role: facade publique stable et additive
input_contract: input explicite et valide
output_contract: output stable et deterministic
boundary: implementation privee sous vitte/$PKG/internal/*
versioning: additive-only, no breaking removals in stable
api_surface_stable: true
diagnostics: ${DIAG_PREFIX}0001..${DIAG_PREFIX}0099
compat_policy: additive-only
internal_exports: forbidden
api_version: v1
compat_layer: none
removal_policy: major-only
>>>
EOF2

cat > "$PKG_DIR/internal/runtime.vit" <<EOF2
<<<
runtime.vit
package vitte/$PKG/internal/runtime
>>>

space vitte/$PKG/internal/runtime
EOF2

cat > "$CONTRACT_DIR/$PKG.exports" <<EOF2
# generated placeholder
EOF2
cat > "$CONTRACT_DIR/$PKG.facade.api" <<EOF2
# generated placeholder
EOF2
cat > "$CONTRACT_DIR/$PKG.contract.lock.json" <<EOF2
{
  "package": "vitte/$PKG",
  "schema_version": "1.0"
}
EOF2

echo "[scaffold-package] generated $PKG (strict=$STRICT_FACADE doctor=$WITH_DOCTOR quickfix=$WITH_QUICKFIX diag=${DIAG_PREFIX})"
