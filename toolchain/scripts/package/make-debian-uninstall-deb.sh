#!/usr/bin/env bash
# ============================================================
# vitte — Debian uninstall .deb builder (nopayload)
# Installs a small package that removes Vitte files from system.
# ============================================================

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
VERSION="${VERSION:-2.1.2}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte-uninstall}"
MAINTAINER="${MAINTAINER:-Vitte Team <maintainers@vitte.dev>}"
DESCRIPTION="${DESCRIPTION:-Uninstaller package for Vitte toolchain}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkg/out}"
ARCH="${ARCH:-$(dpkg --print-architecture 2>/dev/null || echo amd64)}"

STAGE_BASE="$ROOT_DIR/.debstage/${PACKAGE_NAME}-${VERSION}"
STAGE_ROOT="$STAGE_BASE/root"
DEBIAN_DIR="$STAGE_ROOT/DEBIAN"
OUT_DEB="$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"

log() { printf "[make-debian-uninstall-deb] %s\n" "$*"; }
die() { printf "[make-debian-uninstall-deb][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

require_tools() {
  has dpkg-deb || die "dpkg-deb is required"
}

prepare_stage() {
  rm -rf "$STAGE_BASE"
  mkdir -p "$DEBIAN_DIR" "$OUT_DIR"
}

write_control() {
  cat > "$DEBIAN_DIR/control" <<EOF_CTRL
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: admin
Priority: optional
Architecture: ${ARCH}
Maintainer: ${MAINTAINER}
Depends: bash, coreutils, dpkg
Recommends: apt
Conflicts: vitte
Replaces: vitte
Description: ${DESCRIPTION}
 This package removes Vitte binaries, runtime payload, shell wrappers,
 desktop launchers, icons, manpages and completion files.
EOF_CTRL
}

write_maintainers() {
  cat > "$DEBIAN_DIR/postinst" <<'EOF_POSTINST'
#!/usr/bin/env bash
set -euo pipefail

log() { printf "[vitte-uninstall] %s\n" "$*"; }

remove_path() {
  local p="$1"
  if [ -e "$p" ] || [ -L "$p" ]; then
    rm -rf "$p"
    log "removed: $p"
  fi
}

if command -v dpkg-query >/dev/null 2>&1; then
  if dpkg-query -W -f='${Status}' vitte 2>/dev/null | grep -q "install ok installed"; then
    log "purging installed package: vitte"
    dpkg --remove --force-all vitte >/dev/null 2>&1 || true
    dpkg --purge --force-all vitte >/dev/null 2>&1 || true
  fi
fi

remove_path "/usr/local/bin/vitte"
remove_path "/usr/local/bin/vittec"
remove_path "/usr/local/bin/vitte-linker"
remove_path "/usr/local/bin/vitte-ide"
remove_path "/usr/local/bin/vitte-ide-gtk"
remove_path "/usr/local/bin/vitte-editor-gtk"
remove_path "/usr/local/bin/vitte-editor"
remove_path "/usr/local/libexec/vitte"
remove_path "/usr/local/share/vitte"
remove_path "/usr/local/share/man/man1/vitte.1"
remove_path "/usr/local/share/man/man1/vittec.1"
remove_path "/usr/local/share/man/man1/vitte-linker.1"
remove_path "/usr/local/share/man/man1/vitte-editor.1"
remove_path "/usr/local/etc/bash_completion.d/vitte"
remove_path "/usr/local/share/zsh/site-functions/_vitte"
remove_path "/usr/local/share/fish/vendor_completions.d/vitte.fish"
remove_path "/usr/share/applications/vitte.desktop"
remove_path "/usr/share/icons/hicolor/scalable/apps/vitte.svg"
remove_path "/usr/share/pixmaps/vitte.svg"
remove_path "/usr/share/doc/vitte"

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database -q /usr/share/applications >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -q /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi

log "uninstall complete"
exit 0
EOF_POSTINST
  chmod 0755 "$DEBIAN_DIR/postinst"

  cat > "$DEBIAN_DIR/prerm" <<'EOF_PRERM'
#!/usr/bin/env bash
set -euo pipefail
exit 0
EOF_PRERM
  chmod 0755 "$DEBIAN_DIR/prerm"

  cat > "$DEBIAN_DIR/postrm" <<'EOF_POSTRM'
#!/usr/bin/env bash
set -euo pipefail
exit 0
EOF_POSTRM
  chmod 0755 "$DEBIAN_DIR/postrm"
}

build_deb() {
  rm -f "$OUT_DEB"
  if dpkg-deb --help 2>/dev/null | grep -q -- '--root-owner-group'; then
    dpkg-deb --build --root-owner-group "$STAGE_ROOT" "$OUT_DEB"
  else
    dpkg-deb --build "$STAGE_ROOT" "$OUT_DEB"
  fi
  log "wrote $OUT_DEB"
  ls -lh "$OUT_DEB"
}

main() {
  require_tools
  prepare_stage
  write_control
  write_maintainers
  build_deb
}

main "$@"
