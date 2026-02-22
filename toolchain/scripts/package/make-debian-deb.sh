#!/usr/bin/env bash
# ============================================================
# vitte — Debian .deb builder (parity with macOS pkg payload)
# Includes: binaries, std/packages, runtime sources, editors,
# manpages, shell completions, env helper, postinst checks.
# ============================================================

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"

VERSION="${VERSION:-2.1.1}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"
MAINTAINER="${MAINTAINER:-Vitte Team <maintainers@vitte.dev>}"
DESCRIPTION="${DESCRIPTION:-Vitte systems language toolchain}"
HOMEPAGE="${HOMEPAGE:-https://vitte.netlify.app/}"
SECTION="${SECTION:-devel}"
PRIORITY="${PRIORITY:-optional}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"
ARCH="${ARCH:-$(dpkg --print-architecture 2>/dev/null || echo amd64)}"
DEPENDS="${DEPENDS:-bash, libc6, libstdc++6, libssl3 | libssl1.1, libcurl4}"

STAGE_BASE="$ROOT_DIR/.debstage/${PACKAGE_NAME}-${VERSION}"
STAGE_ROOT="$STAGE_BASE/root"
DEBIAN_DIR="$STAGE_ROOT/DEBIAN"
OUT_DEB="$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"

log() { printf "[make-debian-deb] %s\n" "$*"; }
die() { printf "[make-debian-deb][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

resolve_bin() {
  local name="$1"
  local a="$ROOT_DIR/bin/$name"
  local b="$ROOT_DIR/target/bin/$name"
  if [ -x "$a" ]; then printf "%s\n" "$a"; return 0; fi
  if [ -x "$b" ]; then printf "%s\n" "$b"; return 0; fi
  return 1
}

require_tools() {
  has dpkg-deb || die "dpkg-deb is required"
  has rsync || die "rsync is required"
  has python3 || die "python3 is required"
}

stage_payload() {
  local vitte_bin
  vitte_bin="$(resolve_bin vitte || true)"
  [ -n "$vitte_bin" ] || die "vitte binary not found (expected bin/vitte or target/bin/vitte)"

  mkdir -p "$STAGE_ROOT/usr/local/libexec/vitte" "$STAGE_ROOT/usr/local/bin"
  install -m 0755 "$vitte_bin" "$STAGE_ROOT/usr/local/libexec/vitte/vitte"

  cat > "$STAGE_ROOT/usr/local/bin/vitte" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
if [ -z "${VITTE_ROOT:-}" ]; then
  export VITTE_ROOT="/usr/local/share/vitte"
fi
exec /usr/local/libexec/vitte/vitte "$@"
EOF
  chmod 0755 "$STAGE_ROOT/usr/local/bin/vitte"

  for legacy in vittec vitte-linker; do
    local src
    src="$(resolve_bin "$legacy" || true)"
    if [ -n "$src" ]; then
      install -m 0755 "$src" "$STAGE_ROOT/usr/local/libexec/vitte/$legacy"
      ln -sfn "../libexec/vitte/$legacy" "$STAGE_ROOT/usr/local/bin/$legacy"
    else
      ln -sfn "vitte" "$STAGE_ROOT/usr/local/bin/$legacy"
    fi
  done

  mkdir -p "$STAGE_ROOT/usr/local/share/vitte/src/vitte"
  mkdir -p "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends"
  mkdir -p "$STAGE_ROOT/usr/local/share/vitte/editors"
  mkdir -p "$STAGE_ROOT/usr/local/share/vitte/completions"

  rsync -a "$ROOT_DIR/src/vitte/packages/" "$STAGE_ROOT/usr/local/share/vitte/src/vitte/packages/"
  rsync -a "$ROOT_DIR/src/compiler/backends/runtime/" "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends/runtime/"

  mkdir -p "$STAGE_ROOT/usr/local/share/vitte/editors/vim" \
           "$STAGE_ROOT/usr/local/share/vitte/editors/emacs" \
           "$STAGE_ROOT/usr/local/share/vitte/editors/nano"
  rsync -a "$ROOT_DIR/editors/vim/" "$STAGE_ROOT/usr/local/share/vitte/editors/vim/"
  rsync -a "$ROOT_DIR/editors/emacs/" "$STAGE_ROOT/usr/local/share/vitte/editors/emacs/"
  rsync -a "$ROOT_DIR/editors/nano/" "$STAGE_ROOT/usr/local/share/vitte/editors/nano/"
  [ -f "$ROOT_DIR/editors/README.md" ] && install -m 0644 "$ROOT_DIR/editors/README.md" "$STAGE_ROOT/usr/local/share/vitte/editors/README.md"

  install -m 0644 "$ROOT_DIR/toolchain/scripts/install/templates/env.sh" "$STAGE_ROOT/usr/local/share/vitte/env.sh"

  mkdir -p "$STAGE_ROOT/usr/local/share/man/man1"
  for m in vitte.1 vittec.1 vitte-linker.1; do
    [ -f "$ROOT_DIR/man/$m" ] && install -m 0644 "$ROOT_DIR/man/$m" "$STAGE_ROOT/usr/local/share/man/man1/$m"
  done

  mkdir -p "$STAGE_ROOT/usr/local/etc/bash_completion.d"
  mkdir -p "$STAGE_ROOT/usr/local/share/zsh/site-functions"
  mkdir -p "$STAGE_ROOT/usr/local/share/fish/vendor_completions.d"
  mkdir -p "$STAGE_ROOT/usr/local/share/vitte/completions/bash" \
           "$STAGE_ROOT/usr/local/share/vitte/completions/zsh" \
           "$STAGE_ROOT/usr/local/share/vitte/completions/fish"

  install -m 0644 "$ROOT_DIR/completions/bash/vitte" "$STAGE_ROOT/usr/local/etc/bash_completion.d/vitte"
  install -m 0644 "$ROOT_DIR/completions/zsh/_vitte" "$STAGE_ROOT/usr/local/share/zsh/site-functions/_vitte"
  install -m 0644 "$ROOT_DIR/completions/fish/vitte.fish" "$STAGE_ROOT/usr/local/share/fish/vendor_completions.d/vitte.fish"
  install -m 0644 "$ROOT_DIR/completions/bash/vitte" "$STAGE_ROOT/usr/local/share/vitte/completions/bash/vitte"
  install -m 0644 "$ROOT_DIR/completions/zsh/_vitte" "$STAGE_ROOT/usr/local/share/vitte/completions/zsh/_vitte"
  install -m 0644 "$ROOT_DIR/completions/fish/vitte.fish" "$STAGE_ROOT/usr/local/share/vitte/completions/fish/vitte.fish"
}

write_control_files() {
  mkdir -p "$DEBIAN_DIR"

  cat > "$DEBIAN_DIR/control" <<EOF
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: ${SECTION}
Priority: ${PRIORITY}
Architecture: ${ARCH}
Maintainer: ${MAINTAINER}
Depends: ${DEPENDS}
Homepage: ${HOMEPAGE}
Description: ${DESCRIPTION}
 Unified Vitte toolchain package including binary, runtime sources,
 standard packages, editors support, manpages and shell completions.
EOF

  cat > "$DEBIAN_DIR/postinst" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

install_user_editor_support() {
  local user_name="$1"
  [ -n "$user_name" ] || return 0
  [ "$user_name" != "root" ] || return 0
  local home_dir
  home_dir="$(getent passwd "$user_name" | cut -d: -f6 || true)"
  [ -n "$home_dir" ] || return 0
  [ -d "$home_dir" ] || return 0

  local editor_root="/usr/local/share/vitte/editors"
  [ -d "$editor_root" ] || return 0

  mkdir -p "$home_dir/.vim/syntax" "$home_dir/.vim/indent" "$home_dir/.vim/ftdetect" "$home_dir/.vim/ftplugin" "$home_dir/.vim/compiler"
  install -m 0644 "$editor_root/vim/vitte.vim" "$home_dir/.vim/syntax/vitte.vim"
  install -m 0644 "$editor_root/vim/indent/vitte.vim" "$home_dir/.vim/indent/vitte.vim"
  install -m 0644 "$editor_root/vim/ftdetect/vitte.vim" "$home_dir/.vim/ftdetect/vitte.vim"
  install -m 0644 "$editor_root/vim/ftplugin/vitte.vim" "$home_dir/.vim/ftplugin/vitte.vim"
  install -m 0644 "$editor_root/vim/compiler/vitte.vim" "$home_dir/.vim/compiler/vitte.vim"

  mkdir -p "$home_dir/.emacs.d/lisp"
  install -m 0644 "$editor_root/emacs/vitte-mode.el" "$home_dir/.emacs.d/lisp/vitte-mode.el"
  install -m 0644 "$editor_root/emacs/vitte-indent.el" "$home_dir/.emacs.d/lisp/vitte-indent.el"
  local emacs_init="$home_dir/.emacs.d/init.el"
  if [ ! -f "$emacs_init" ]; then
    emacs_init="$home_dir/.emacs"
    touch "$emacs_init"
  fi
  if ! grep -q 'vitte-mode setup' "$emacs_init" 2>/dev/null; then
    cat >> "$emacs_init" <<'EINIT'
;; vitte-mode setup (managed by vitte deb)
(add-to-list 'load-path "~/.emacs.d/lisp")
(autoload 'vitte-mode "vitte-mode" nil t)
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
EINIT
  fi

  mkdir -p "$home_dir/.config/nano"
  install -m 0644 "$editor_root/nano/vitte.nanorc" "$home_dir/.config/nano/vitte.nanorc"
  touch "$home_dir/.nanorc"
  if ! grep -q 'include "~/.config/nano/vitte.nanorc"' "$home_dir/.nanorc"; then
    printf '\ninclude "~/.config/nano/vitte.nanorc"\n' >> "$home_dir/.nanorc"
  fi

  chown -R "$user_name" "$home_dir/.vim" "$home_dir/.emacs.d" "$home_dir/.config/nano" "$home_dir/.nanorc" >/dev/null 2>&1 || true
}

if [ "${1:-}" = "configure" ]; then
  if ! /usr/local/bin/vitte --help >/dev/null 2>&1; then
    echo "[vitte deb] postinst check failed: vitte --help" >&2
    exit 1
  fi

  TMP="/tmp/vitte_deb_verify_$$.vit"
  cat > "$TMP" <<'VEOF'
use vitte/core as core_pkg

entry main at core/app {
  give 0
}
VEOF
  if ! /usr/local/bin/vitte check "$TMP" >/dev/null 2>&1; then
    echo "[vitte deb] postinst check failed: package import test" >&2
    rm -f "$TMP"
    exit 1
  fi
  rm -f "$TMP"

  for p in \
    /usr/local/etc/bash_completion.d/vitte \
    /usr/local/share/zsh/site-functions/_vitte \
    /usr/local/share/fish/vendor_completions.d/vitte.fish
  do
    if [ ! -f "$p" ]; then
      echo "[vitte deb] postinst check failed: missing completion $p" >&2
      exit 1
    fi
  done

  if [ -n "${VITTE_SETUP_USER:-}" ]; then
    install_user_editor_support "$VITTE_SETUP_USER"
  elif [ -n "${SUDO_USER:-}" ]; then
    install_user_editor_support "$SUDO_USER"
  fi

  cat <<MSG
[vitte deb] install complete
[vitte deb] binary: /usr/local/bin/vitte
[vitte deb] root:   /usr/local/share/vitte
[vitte deb] man:    /usr/local/share/man/man1/vitte.1
[vitte deb] env:    source /usr/local/share/vitte/env.sh
MSG
fi

exit 0
EOF
  chmod 0755 "$DEBIAN_DIR/postinst"

  cat > "$DEBIAN_DIR/prerm" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
exit 0
EOF
  chmod 0755 "$DEBIAN_DIR/prerm"

  cat > "$DEBIAN_DIR/postrm" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
exit 0
EOF
  chmod 0755 "$DEBIAN_DIR/postrm"
}

main() {
  require_tools
  cd "$ROOT_DIR"
  mkdir -p "$OUT_DIR"
  rm -rf "$STAGE_BASE"
  mkdir -p "$STAGE_ROOT"

  if [ -f "$ROOT_DIR/tools/generate_completions.py" ]; then
    python3 "$ROOT_DIR/tools/generate_completions.py"
    python3 "$ROOT_DIR/tools/generate_completions.py" --check
  fi

  stage_payload
  write_control_files

  find "$STAGE_ROOT" -name '.DS_Store' -type f -delete || true

  if dpkg-deb --help 2>/dev/null | grep -q -- '--root-owner-group'; then
    dpkg-deb --build --root-owner-group "$STAGE_ROOT" "$OUT_DEB"
  else
    dpkg-deb --build "$STAGE_ROOT" "$OUT_DEB"
  fi

  log "wrote $OUT_DEB"
  ls -lh "$OUT_DEB"
}

main "$@"
