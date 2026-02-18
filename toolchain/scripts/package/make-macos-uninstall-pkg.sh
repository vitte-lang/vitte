#!/usr/bin/env bash
# ============================================================
# vitte — macOS uninstall .pkg builder (nopayload)
# Removes system install paths.
# Optional user editor cleanup:
#   REMOVE_USER_EDITOR_CONFIG=1 sudo installer -pkg ... -target /
# ============================================================

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
VERSION="${VERSION:-2.1.1}"
IDENTIFIER="${IDENTIFIER:-org.vitte.toolchain.uninstall}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"

STAGE_BASE="$ROOT_DIR/.pkgstage/vitte-uninstall-$VERSION"
SCRIPTS_DIR="$STAGE_BASE/scripts"
OUT_PKG="$OUT_DIR/vitte-uninstall-$VERSION.pkg"

log() { printf "[make-macos-uninstall-pkg] %s\n" "$*"; }

cd "$ROOT_DIR"
mkdir -p "$OUT_DIR"
rm -rf "$STAGE_BASE"
mkdir -p "$SCRIPTS_DIR"

cat > "$SCRIPTS_DIR/preinstall" <<'EOF'
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

cleanup_user_editor_config() {
  local user_name="$1"
  local home_dir="$2"
  [ -n "$user_name" ] || return 0
  [ -d "$home_dir" ] || return 0

  # Remove known editor files only.
  remove_path "$home_dir/.vim/syntax/vitte.vim"
  remove_path "$home_dir/.vim/indent/vitte.vim"
  remove_path "$home_dir/.vim/ftdetect/vitte.vim"
  remove_path "$home_dir/.vim/ftplugin/vitte.vim"
  remove_path "$home_dir/.vim/compiler/vitte.vim"
  remove_path "$home_dir/.emacs.d/lisp/vitte-mode.el"
  remove_path "$home_dir/.emacs.d/lisp/vitte-indent.el"
  remove_path "$home_dir/.config/nano/vitte.nanorc"

  # Guard rail: only remove the exact include line from .nanorc.
  local nanorc="$home_dir/.nanorc"
  if [ -f "$nanorc" ]; then
    cp "$nanorc" "$nanorc.vitte-uninstall.bak"
    grep -v '^include "~/.config/nano/vitte.nanorc"$' "$nanorc.vitte-uninstall.bak" > "$nanorc.tmp" || true
    mv "$nanorc.tmp" "$nanorc"
    log "updated: $nanorc (backup: $nanorc.vitte-uninstall.bak)"
  fi

  # Remove vitte-mode auto-load block from Emacs init files.
  for emacs_init in "$home_dir/.emacs.d/init.el" "$home_dir/.emacs"; do
    if [ -f "$emacs_init" ]; then
      cp "$emacs_init" "$emacs_init.vitte-uninstall.bak"
      grep -v '^;; vitte-mode setup (managed by vitte pkg)$' "$emacs_init.vitte-uninstall.bak" \
        | grep -v "^(add-to-list 'load-path \"~/.emacs.d/lisp\")$" \
        | grep -v "^(autoload 'vitte-mode \"vitte-mode\" nil t)$" \
        | grep -v "^(add-to-list 'auto-mode-alist '(\"\\\\\\\\.vit\\\\\\\\'\" . vitte-mode))$" \
        > "$emacs_init.tmp" || true
      mv "$emacs_init.tmp" "$emacs_init"
      log "updated: $emacs_init (backup: $emacs_init.vitte-uninstall.bak)"
    fi
  done
}

# System paths
remove_path "/usr/local/bin/vitte"
remove_path "/usr/local/bin/vittec"
remove_path "/usr/local/bin/vitte-linker"
remove_path "/usr/local/libexec/vitte"
remove_path "/usr/local/share/vitte"
remove_path "/usr/local/share/man/man1/vitte.1"
remove_path "/usr/local/share/man/man1/vittec.1"
remove_path "/usr/local/share/man/man1/vitte-linker.1"
remove_path "/usr/local/etc/bash_completion.d/vitte"
remove_path "/usr/local/share/zsh/site-functions/_vitte"
remove_path "/usr/local/share/fish/vendor_completions.d/vitte.fish"

rmdir "/usr/local/share/man/man1" 2>/dev/null || true
rmdir "/usr/local/share/man" 2>/dev/null || true
rmdir "/usr/local/libexec" 2>/dev/null || true

if [ "${REMOVE_USER_EDITOR_CONFIG:-0}" = "1" ]; then
  # Optional user config cleanup with safeguards.
  console_user="$(stat -f%Su /dev/console 2>/dev/null || true)"
  if [ -n "$console_user" ] && [ "$console_user" != "root" ]; then
    console_home="$(dscl . -read "/Users/$console_user" NFSHomeDirectory 2>/dev/null | awk '{print $2}' || true)"
    cleanup_user_editor_config "$console_user" "$console_home"
  fi
  if [ -n "${SUDO_USER:-}" ] && [ "${SUDO_USER}" != "root" ] && [ "${SUDO_USER}" != "${console_user:-}" ]; then
    sudo_home="$(dscl . -read "/Users/$SUDO_USER" NFSHomeDirectory 2>/dev/null | awk '{print $2}' || true)"
    cleanup_user_editor_config "$SUDO_USER" "$sudo_home"
  fi
fi

if command -v pkgutil >/dev/null 2>&1; then
  pkgutil --forget org.vitte.toolchain >/dev/null 2>&1 || true
fi

log "uninstall complete"
exit 0
EOF

chmod 0755 "$SCRIPTS_DIR/preinstall"

pkgbuild \
  --nopayload \
  --scripts "$SCRIPTS_DIR" \
  --identifier "$IDENTIFIER" \
  --version "$VERSION" \
  "$OUT_PKG"

log "wrote $OUT_PKG"
ls -lh "$OUT_PKG"
