#!/bin/sh
set -eu

VITTE_BIN=${VITTE_BIN:-vitte}
case "$VITTE_BIN" in
  /*) ;;
  */*)
    vitte_dir=$(CDPATH= cd -- "$(dirname "$VITTE_BIN")" && pwd)
    VITTE_BIN=$vitte_dir/$(basename "$VITTE_BIN")
    ;;
esac

if [ -n "${WORKDIR:-}" ]; then
  mkdir -p "$WORKDIR"
else
  WORKDIR=$(mktemp -d "${TMPDIR:-/tmp}/vitte-real-install-smoke.XXXXXX")
  cleanup() { rm -rf -- "$WORKDIR"; }
  trap cleanup EXIT HUP INT TERM
fi

cat > "$WORKDIR/smoke.vit" <<'VIT'
proc main() -> int {
  give 0;
}
VIT

cd "$WORKDIR"

# Required post-install contract:
case "$VITTE_BIN" in
  /*)
    doctor=$(dirname "$VITTE_BIN")/vitte-installer-doctor
    [ ! -x "$doctor" ] || "$doctor"
    ;;
  *)
    if command -v vitte-installer-doctor >/dev/null 2>&1; then
      vitte-installer-doctor
    fi
    ;;
esac
"$VITTE_BIN" --version >/dev/null
"$VITTE_BIN" --help >/dev/null
"$VITTE_BIN" check smoke.vit
"$VITTE_BIN" compile smoke.vit -o smoke

smoke_program=./smoke
[ -x "$smoke_program" ] || smoke_program=./smoke.exe
[ -x "$smoke_program" ] || {
  printf '[real-install-smoke][error] compiler produced no executable in %s\n' "$WORKDIR" >&2
  exit 1
}
"$smoke_program"

printf '[real-install-smoke] OK bin=%s workdir=%s\n' "$VITTE_BIN" "$WORKDIR"
