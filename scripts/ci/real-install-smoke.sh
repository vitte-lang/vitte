#!/bin/sh
set -eu

VITTE_BIN=${VITTE_BIN:-vitte}
WORKDIR=${WORKDIR:-${TMPDIR:-/tmp}/vitte-real-install-smoke-$$}
mkdir -p "$WORKDIR"

cat > "$WORKDIR/smoke.vit" <<'VIT'
proc main() -> int {
  give 0
}
VIT

cd "$WORKDIR"

# Required post-install contract:
"$VITTE_BIN" --help >/dev/null
"$VITTE_BIN" check smoke.vit
"$VITTE_BIN" build smoke.vit -o smoke

if [ -x ./smoke ]; then
  ./smoke
fi

printf '[real-install-smoke] OK bin=%s workdir=%s\n' "$VITTE_BIN" "$WORKDIR"
