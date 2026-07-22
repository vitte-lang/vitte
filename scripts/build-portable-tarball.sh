#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=build-portable-tarball
. "$ROOT_DIR/scripts_build/common.sh"
scripts_build_parse_common_flags "$@"
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
case "$OUT_DIR" in /*) ;; *) OUT_DIR=$ROOT_DIR/$OUT_DIR ;; esac
PLATFORM=${PLATFORM:-linux}
ARCH=${ARCH:-$(uname -m 2>/dev/null || printf unknown)}
PACKAGE_DIR=${PACKAGE_DIR:-vitte-$VERSION-portable-$PLATFORM-$ARCH}
BUILD_DIR=${BUILD_DIR:-$ROOT_DIR/target/portable-$PLATFORM-$ARCH}

scripts_build_maybe_help "usage: build-portable-tarball.sh [--dry-run]"
scripts_build_maybe_dry_run "would build portable tarball platform=$PLATFORM arch=$ARCH version=$VERSION out=$OUT_DIR"

if [ "$ARCH" = all ]; then
  for portable_arch in amd64 i386 arm64 armv7 armv6 riscv64; do
    VERSION=$VERSION OUT_DIR=$OUT_DIR PLATFORM=$PLATFORM ARCH=$portable_arch "$0"
  done
  exit 0
fi

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/stage" "$BUILD_DIR/$PACKAGE_DIR" "$OUT_DIR"

VERSION=$VERSION "$ROOT_DIR/scripts_build/stage-installer-payload.sh" \
  "$BUILD_DIR/stage/root" "$PLATFORM" "$ARCH" unix >/dev/null

portable_root=$BUILD_DIR/$PACKAGE_DIR
scripts_build_copy_tree "$BUILD_DIR/stage/root/usr/local/bin" "$portable_root/bin"
scripts_build_copy_tree "$BUILD_DIR/stage/root/usr/local/libexec" "$portable_root/libexec"
scripts_build_copy_tree "$BUILD_DIR/stage/root/usr/local/share" "$portable_root/share"

for command in vitte vittec vittec0; do
  cat > "$portable_root/bin/$command" <<EOF
#!/bin/sh
set -eu
self=\$0
case "\$self" in
  */*) ;;
  *)
    found=\$(command -v "\$self" 2>/dev/null || true)
    [ -n "\$found" ] || { printf '%s\n' "portable Vitte wrapper must be invoked through an installed path" >&2; exit 127; }
    self=\$found
    ;;
esac
bindir=\${self%/*}
case "\$bindir" in
  /*) ;;
  *) bindir=\$(CDPATH= cd "\$bindir" && pwd) ;;
esac
root=\${bindir%/bin}
export VITTE_ROOT=\${VITTE_ROOT:-\$root/share/vitte}
exec "\$root/libexec/vitte/$command" "\$@"
EOF
  chmod 0755 "$portable_root/bin/$command"
done

cat > "$portable_root/README.portable" <<EOF
Vitte $VERSION portable archive for $PLATFORM/$ARCH.

Run commands through absolute or relative paths, for example:

  ./bin/vitte --version

The wrappers set VITTE_ROOT to ./share/vitte and do not require a system PATH
entry when invoked through a path.
EOF

archive=$OUT_DIR/$PACKAGE_DIR.tar.gz
scripts_build_tar_gz "$archive" "$BUILD_DIR" "$PACKAGE_DIR"
scripts_build_sha256_write "$archive"
scripts_build_write_artifact_manifest "$archive" "$PLATFORM" "$ARCH" "$VERSION"

printf '[build-portable-tarball] wrote %s (%s bytes)\n' \
  "$archive" "$(wc -c < "$archive" | tr -d ' ')"
