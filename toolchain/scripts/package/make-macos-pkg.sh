#!/usr/bin/env bash
# ============================================================
# vitte — macOS unified .pkg builder
# Includes: binaries, std/packages, runtime sources, editors,
# manpages, shell completions, env helper.
# ============================================================

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
VERSION="${VERSION:-2.1.1}"
IDENTIFIER="${IDENTIFIER:-org.vitte.toolchain}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"

STAGE_BASE="$ROOT_DIR/.pkgstage/vitte-$VERSION"
STAGE_ROOT="$STAGE_BASE/root"
SCRIPTS_DIR="$STAGE_BASE/scripts"
OUT_PKG="$OUT_DIR/vitte-$VERSION.pkg"

log() { printf "[make-macos-pkg] %s\n" "$*"; }
die() { printf "[make-macos-pkg][error] %s\n" "$*" >&2; exit 1; }

resolve_bin() {
  local name="$1"
  local a="$ROOT_DIR/bin/$name"
  local b="$ROOT_DIR/target/bin/$name"
  if [ -x "$a" ]; then printf "%s\n" "$a"; return 0; fi
  if [ -x "$b" ]; then printf "%s\n" "$b"; return 0; fi
  return 1
}

cd "$ROOT_DIR"
mkdir -p "$OUT_DIR"
rm -rf "$STAGE_BASE"
mkdir -p "$STAGE_ROOT" "$SCRIPTS_DIR"

# Keep shell completion artifacts in sync from the unified spec.
if [ -f "$ROOT_DIR/tools/generate_completions.py" ]; then
  python3 "$ROOT_DIR/tools/generate_completions.py"
  python3 "$ROOT_DIR/tools/generate_completions.py" --check
fi

VITTE_BIN="$(resolve_bin vitte || true)"
[ -n "$VITTE_BIN" ] || die "vitte binary not found (expected bin/vitte or target/bin/vitte)"

mkdir -p "$STAGE_ROOT/usr/local/libexec/vitte" "$STAGE_ROOT/usr/local/bin"
install -m 0755 "$VITTE_BIN" "$STAGE_ROOT/usr/local/libexec/vitte/vitte"

# Primary wrapper with VITTE_ROOT default.
cat > "$STAGE_ROOT/usr/local/bin/vitte" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
if [ -z "${VITTE_ROOT:-}" ]; then
  export VITTE_ROOT="/usr/local/share/vitte"
fi
exec /usr/local/libexec/vitte/vitte "$@"
EOF
chmod 0755 "$STAGE_ROOT/usr/local/bin/vitte"

# vittec and vitte-linker compatibility.
for legacy in vittec vitte-linker; do
  src="$(resolve_bin "$legacy" || true)"
  if [ -n "$src" ]; then
    install -m 0755 "$src" "$STAGE_ROOT/usr/local/libexec/vitte/$legacy"
    ln -sfn "../libexec/vitte/$legacy" "$STAGE_ROOT/usr/local/bin/$legacy"
  else
    ln -sfn "vitte" "$STAGE_ROOT/usr/local/bin/$legacy"
  fi
done

# Share tree.
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/src/vitte"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/editors"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/completions"

rsync -a "$ROOT_DIR/src/vitte/packages/" "$STAGE_ROOT/usr/local/share/vitte/src/vitte/packages/"
rsync -a "$ROOT_DIR/src/compiler/backends/runtime/" "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends/runtime/"

# Editors (Vim/Emacs/Nano only).
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/editors/vim" \
         "$STAGE_ROOT/usr/local/share/vitte/editors/emacs" \
         "$STAGE_ROOT/usr/local/share/vitte/editors/nano"
rsync -a "$ROOT_DIR/editors/vim/" "$STAGE_ROOT/usr/local/share/vitte/editors/vim/"
rsync -a "$ROOT_DIR/editors/emacs/" "$STAGE_ROOT/usr/local/share/vitte/editors/emacs/"
rsync -a "$ROOT_DIR/editors/nano/" "$STAGE_ROOT/usr/local/share/vitte/editors/nano/"
[ -f "$ROOT_DIR/editors/README.md" ] && install -m 0644 "$ROOT_DIR/editors/README.md" "$STAGE_ROOT/usr/local/share/vitte/editors/README.md"

# Environment helper.
install -m 0644 "$ROOT_DIR/toolchain/scripts/install/templates/env.sh" "$STAGE_ROOT/usr/local/share/vitte/env.sh"

# Man pages.
mkdir -p "$STAGE_ROOT/usr/local/share/man/man1"
for m in vitte.1 vittec.1 vitte-linker.1; do
  [ -f "$ROOT_DIR/man/$m" ] && install -m 0644 "$ROOT_DIR/man/$m" "$STAGE_ROOT/usr/local/share/man/man1/$m"
done

# Shell completions (installed in standard lookup paths + share copy).
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

find "$STAGE_ROOT" -name '.DS_Store' -type f -delete || true

# Postinstall verification.
cat > "$SCRIPTS_DIR/postinstall" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

install_user_editor_support() {
  local user_name="$1"
  local home_dir="$2"
  local editor_root="/usr/local/share/vitte/editors"
  [ -n "$user_name" ] || return 0
  [ -d "$home_dir" ] || return 0
  [ -d "$editor_root" ] || return 0

  # Vim syntax/ftplugin/compiler.
  mkdir -p "$home_dir/.vim/syntax" "$home_dir/.vim/indent" "$home_dir/.vim/ftdetect" "$home_dir/.vim/ftplugin" "$home_dir/.vim/compiler"
  install -m 0644 "$editor_root/vim/vitte.vim" "$home_dir/.vim/syntax/vitte.vim"
  install -m 0644 "$editor_root/vim/indent/vitte.vim" "$home_dir/.vim/indent/vitte.vim"
  install -m 0644 "$editor_root/vim/ftdetect/vitte.vim" "$home_dir/.vim/ftdetect/vitte.vim"
  install -m 0644 "$editor_root/vim/ftplugin/vitte.vim" "$home_dir/.vim/ftplugin/vitte.vim"
  install -m 0644 "$editor_root/vim/compiler/vitte.vim" "$home_dir/.vim/compiler/vitte.vim"

  # Emacs mode files + auto-load block.
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
;; vitte-mode setup (managed by vitte pkg)
(add-to-list 'load-path "~/.emacs.d/lisp")
(autoload 'vitte-mode "vitte-mode" nil t)
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
EINIT
  fi

  # Nano syntax include.
  mkdir -p "$home_dir/.config/nano"
  install -m 0644 "$editor_root/nano/vitte.nanorc" "$home_dir/.config/nano/vitte.nanorc"
  touch "$home_dir/.nanorc"
  if ! grep -q 'include "~/.config/nano/vitte.nanorc"' "$home_dir/.nanorc"; then
    printf '\ninclude "~/.config/nano/vitte.nanorc"\n' >> "$home_dir/.nanorc"
  fi

  chown -R "$user_name" "$home_dir/.vim" "$home_dir/.emacs.d" "$home_dir/.config/nano" "$home_dir/.nanorc" >/dev/null 2>&1 || true
}

if ! /usr/local/bin/vitte --help >/dev/null 2>&1; then
  echo "[vitte pkg] postinstall check failed: vitte --help" >&2
  exit 1
fi

TMP="/tmp/vitte_pkg_verify_$$.vit"
cat > "$TMP" <<'VEOF'
use vitte/core as core_pkg

entry main at core/app {
  give 0
}
VEOF

if ! /usr/local/bin/vitte check "$TMP" >/dev/null 2>&1; then
  echo "[vitte pkg] postinstall check failed: package import test" >&2
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
    echo "[vitte pkg] postinstall check failed: missing completion $p" >&2
    exit 1
  fi
done

# Auto-enable editor syntax for current user.
console_user="$(stat -f%Su /dev/console 2>/dev/null || true)"
if [ -n "$console_user" ] && [ "$console_user" != "root" ]; then
  console_home="$(dscl . -read "/Users/$console_user" NFSHomeDirectory 2>/dev/null | awk '{print $2}' || true)"
  install_user_editor_support "$console_user" "$console_home"
fi
if [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != "root" ] && [ "${SUDO_USER}" != "${console_user:-}" ]; then
  sudo_home="$(dscl . -read "/Users/$SUDO_USER" NFSHomeDirectory 2>/dev/null | awk '{print $2}' || true)"
  install_user_editor_support "$SUDO_USER" "$sudo_home"
fi

cat <<MSG
[vitte pkg] install complete
[vitte pkg] binary: /usr/local/bin/vitte
[vitte pkg] root:   /usr/local/share/vitte
[vitte pkg] man:    /usr/local/share/man/man1/vitte.1
[vitte pkg] editors: ~/.vim ~/.emacs.d ~/.config/nano configured
[vitte pkg] zsh:    autoload -U compinit && compinit
[vitte pkg] env:    source /usr/local/share/vitte/env.sh
MSG
EOF
chmod 0755 "$SCRIPTS_DIR/postinstall"

pkgbuild \
  --root "$STAGE_ROOT" \
  --scripts "$SCRIPTS_DIR" \
  --identifier "$IDENTIFIER" \
  --version "$VERSION" \
  --install-location "/" \
  "$OUT_PKG"

log "wrote $OUT_PKG"
ls -lh "$OUT_PKG"
