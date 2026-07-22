#!/bin/sh
set -eu

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

for command in vitte vittec vittec0; do
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
fi

if [ "$status" -eq 0 ]; then
  line "installer status: OK"
else
  line "installer status: FAIL"
fi

exit "$status"
