#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=script-build-install-check
. "$ROOT_DIR/scripts_build/common.sh"

tmp=${TMPDIR:-/tmp}/vitte-script-build-install-check-$$
/bin/rm -rf "$tmp"
/bin/mkdir -p "$tmp/portable/bin" "$tmp/old-path" "$tmp/home"
trap '/bin/rm -rf "$tmp"' EXIT HUP INT TERM

cat > "$tmp/portable/bin/vitte" <<'SH'
#!/bin/sh
set -eu
case "${1:-}" in
  --version)
    printf '%s\n' 'vitte 0.1.0'
    exit 0
    ;;
  --help)
    printf '%s\n' 'Vitte help'
    exit 0
    ;;
  check)
    [ -n "${2:-}" ] || exit 2
    exit 0
    ;;
  build)
    output=main
    while [ "$#" -gt 0 ]; do
      case "$1" in
        -o)
          shift
          output=${1:-main}
          ;;
      esac
      shift || true
    done
    printf '%s\n' '#!/bin/sh' 'exit 0' > "$output"
    /bin/chmod 0755 "$output"
    exit 0
    ;;
esac
exit 0
SH
chmod 0755 "$tmp/portable/bin/vitte"

cat > "$tmp/old-path/vitte" <<'SH'
#!/bin/sh
printf '%s\n' 'old vitte from polluted PATH' >&2
exit 99
SH
chmod 0755 "$tmp/old-path/vitte"

HOME=$tmp/home
PATH=$tmp/old-path:/does/not/exist
VITTE_BIN=$tmp/portable/bin/vitte
VITTE_ROOT=
export HOME PATH VITTE_BIN VITTE_ROOT

detected=$(detect_vitte)
[ "$detected" = "$tmp/portable/bin/vitte" ] ||
  scripts_build_die "detect_vitte used the wrong binary: $detected"

verified=$(verify_vitte "$detected")
[ "$verified" = "$tmp/portable/bin/vitte" ] ||
  scripts_build_die "verify_vitte returned the wrong binary: $verified"

version=$(run_vitte_absolute --version)
[ "$version" = "vitte 0.1.0" ] ||
  scripts_build_die "run_vitte_absolute did not use the detected absolute binary"

VITTE_BIN=
VITTE_ROOT=$tmp/portable
export VITTE_BIN VITTE_ROOT
detected_from_root=$(cd "$tmp" && detect_vitte)
[ "$detected_from_root" = "$tmp/portable/bin/vitte" ] ||
  scripts_build_die "detect_vitte did not resolve VITTE_ROOT/bin/vitte"

VITTE_ROOT=$tmp/portable/share/vitte
export VITTE_ROOT
detected_from_share_root=$(cd "$tmp" && detect_vitte)
[ "$detected_from_share_root" = "$tmp/portable/bin/vitte" ] ||
  scripts_build_die "detect_vitte did not resolve VITTE_ROOT share prefix"

printf '[script-build-install] ok: detect verify absolute-run clean-shell dirty-path\n'
