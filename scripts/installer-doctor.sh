#!/bin/sh
set -eu

PATH=/usr/bin:/bin:${PATH:-}

status=0

line() {
  printf '%s\n' "$*"
}

fail() {
  status=1
  line "FAIL $*"
}

pass() {
  line "OK   $*"
}

self=$0
case "$self" in
  */*) ;;
  *)
    found=$(command -v "$self" 2>/dev/null || true)
    [ -n "$found" ] || {
      fail "cannot resolve command path for $self"
      line "hint: run this command through an absolute path"
      exit "$status"
    }
    self=$found
    ;;
esac

bindir=${self%/*}
case "$bindir" in
  /*) ;;
  *) bindir=$(CDPATH= cd -- "$bindir" && pwd) ;;
esac

prefix=${VITTE_INSTALL_PREFIX:-${bindir%/bin}}
share=${VITTE_ROOT:-$prefix/share/vitte}
libexec=$prefix/libexec/vitte

line "Vitte installer doctor"
line "prefix: $prefix"
line "bin: $bindir"
line "libexec: $libexec"
line "VITTE_ROOT: $share"

case "$prefix" in
  "") fail "empty installation prefix" ;;
  /*) pass "installation prefix is absolute" ;;
  *) fail "installation prefix is not absolute: $prefix" ;;
esac

if [ -n "${VITTE_ROOT:-}" ] && [ "$VITTE_ROOT" != "$prefix/share/vitte" ]; then
  fail "VITTE_ROOT points outside this installation: $VITTE_ROOT"
  line "hint: unset VITTE_ROOT or set it to $prefix/share/vitte"
fi

for command in vitte vittec; do
  wrapper=$bindir/$command
  payload=$libexec/$command
  [ -x "$wrapper" ] && pass "wrapper exists: $wrapper" || fail "missing wrapper: $wrapper"
  [ -x "$payload" ] && pass "payload exists: $payload" || fail "missing payload: $payload"
done

[ -d "$share" ] && pass "share directory exists: $share" || fail "missing share directory: $share"
[ -s "$share/VERSION" ] && pass "version file exists: $share/VERSION" || fail "missing version file: $share/VERSION"
[ -s "$share/INSTALLATION.json" ] && pass "installation manifest exists: $share/INSTALLATION.json" || fail "missing installation manifest: $share/INSTALLATION.json"

if [ -x "$bindir/vitte" ]; then
  if "$bindir/vitte" --version >/dev/null 2>&1; then
    pass "vitte --version runs"
  else
    fail "vitte --version failed"
    line "hint: run $bindir/vitte --version to see the compiler error"
  fi
  if "$bindir/vitte" --help >/dev/null 2>&1; then
    pass "vitte --help runs"
  else
    fail "vitte --help failed"
    line "hint: run $bindir/vitte --help to inspect CLI startup"
  fi

  smoke_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitte-installer-doctor.XXXXXX") || {
    fail "cannot create a temporary smoke directory"
    exit "$status"
  }
  cleanup_smoke() { rm -rf -- "$smoke_dir"; }
  trap cleanup_smoke EXIT HUP INT TERM
  cat > "$smoke_dir/smoke.vit" <<'VITTE_INSTALLER_DOCTOR_SMOKE'
proc main() -> int {
  give 0;
}
VITTE_INSTALLER_DOCTOR_SMOKE

  if (cd "$smoke_dir" && "$bindir/vitte" check smoke.vit >/dev/null 2>&1); then
    pass "vitte check smoke.vit runs"
  else
    fail "vitte check smoke.vit failed"
    line "hint: cd $smoke_dir && $bindir/vitte check smoke.vit"
  fi

  if (cd "$smoke_dir" && "$bindir/vitte" compile smoke.vit -o smoke >/dev/null 2>&1); then
    pass "vitte compile smoke.vit -o smoke runs"
    smoke_program=$smoke_dir/smoke
    [ -x "$smoke_program" ] || smoke_program=$smoke_dir/smoke.exe
    if [ -x "$smoke_program" ]; then
      if "$smoke_program" >/dev/null 2>&1; then
        pass "compiled smoke executable runs"
      else
        fail "compiled smoke executable failed"
      fi
    else
      fail "compiler produced no smoke executable"
    fi
  else
    fail "vitte compile smoke.vit -o smoke failed"
    line "hint: cd $smoke_dir && $bindir/vitte compile smoke.vit -o smoke"
  fi
  cleanup_smoke
  trap - EXIT HUP INT TERM
fi

path_vitte=$(command -v vitte 2>/dev/null || true)
if [ -n "$path_vitte" ] && [ "$path_vitte" != "$bindir/vitte" ]; then
  line "WARN PATH resolves vitte to a different installation: $path_vitte"
  line "hint: put $bindir before older Vitte entries in PATH"
fi

if [ "$status" -eq 0 ]; then
  line "installer status: OK"
else
  line "installer status: FAIL"
fi

exit "$status"
