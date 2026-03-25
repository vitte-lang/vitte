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
OUT_FILE_NAME="${OUT_FILE_NAME:-}"
VITTE_BIN_OVERRIDE="${VITTE_BIN_OVERRIDE:-}"
CHECKSUM_TARGET_BIN="${CHECKSUM_TARGET_BIN:-$ROOT_DIR/target/universal/vitte}"

STAGE_BASE="$ROOT_DIR/.pkgstage/vitte-$VERSION"
STAGE_ROOT="$STAGE_BASE/root"
SCRIPTS_DIR="$STAGE_BASE/scripts"
if [ -n "$OUT_FILE_NAME" ]; then
  OUT_PKG="$OUT_DIR/$OUT_FILE_NAME"
else
  OUT_PKG="$OUT_DIR/vitte-$VERSION.pkg"
fi

log() { printf "[make-macos-pkg] %s\n" "$*"; }
die() { printf "[make-macos-pkg][error] %s\n" "$*" >&2; exit 1; }

sha256_file() {
  local path="$1"
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$path" | awk '{print $1}'
    return
  fi
  shasum -a 256 "$path" | awk '{print $1}'
}

sha512_file() {
  local path="$1"
  if command -v sha512sum >/dev/null 2>&1; then
    sha512sum "$path" | awk '{print $1}'
    return
  fi
  shasum -a 512 "$path" | awk '{print $1}'
}

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
if [ -d "$STAGE_BASE" ]; then
  chmod -R u+w "$STAGE_BASE" >/dev/null 2>&1 || true
fi
rm -rf "$STAGE_BASE"
mkdir -p "$STAGE_ROOT" "$SCRIPTS_DIR"

# Keep shell completion artifacts in sync from the unified spec.
if [ -f "$ROOT_DIR/tools/generate_completions.py" ]; then
  python3 "$ROOT_DIR/tools/generate_completions.py"
  python3 "$ROOT_DIR/tools/generate_completions.py" --check
fi

if [ -n "$VITTE_BIN_OVERRIDE" ] && [ -x "$VITTE_BIN_OVERRIDE" ]; then
  VITTE_BIN="$VITTE_BIN_OVERRIDE"
else
  VITTE_BIN="$(resolve_bin vitte || true)"
fi
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

# Share tree.
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/src/vitte"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/editors"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/completions"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/docs"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/tests/modules/contracts"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/toolchain/scripts/install/templates"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/checksums"
mkdir -p "$STAGE_ROOT/usr/local/share/vitte/manifest"

rsync -a "$ROOT_DIR/src/vitte/packages/" "$STAGE_ROOT/usr/local/share/vitte/src/vitte/packages/"
rsync -a "$ROOT_DIR/src/compiler/backends/runtime/" "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends/runtime/"
rsync -a "$ROOT_DIR/docs/" "$STAGE_ROOT/usr/local/share/vitte/docs/"
rsync -a "$ROOT_DIR/tests/modules/contracts/" "$STAGE_ROOT/usr/local/share/vitte/tests/modules/contracts/"
rsync -a "$ROOT_DIR/toolchain/scripts/install/templates/" "$STAGE_ROOT/usr/local/share/vitte/toolchain/scripts/install/templates/"

# Normalize source payload permissions so postinstall verification can read modules.
find "$STAGE_ROOT/usr/local/share/vitte/src/vitte/packages" -type d -exec chmod 0755 {} +
find "$STAGE_ROOT/usr/local/share/vitte/src/vitte/packages" -type f -exec chmod 0644 {} +
find "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends/runtime" -type d -exec chmod 0755 {} +
find "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends/runtime" -type f -exec chmod 0644 {} +

# Editors (full tree: vim/emacs/nano/geany/tree-sitter/grammar).
rsync -a "$ROOT_DIR/editors/" "$STAGE_ROOT/usr/local/share/vitte/editors/"

# Environment helper.
install -m 0644 "$ROOT_DIR/toolchain/scripts/install/templates/env.sh" "$STAGE_ROOT/usr/local/share/vitte/env.sh"

# Shared metadata files.
install -m 0644 "$ROOT_DIR/LICENSE" "$STAGE_ROOT/usr/local/share/vitte/LICENSE"
install -m 0644 "$ROOT_DIR/CHANGELOG.md" "$STAGE_ROOT/usr/local/share/vitte/CHANGELOG"
install -m 0644 "$ROOT_DIR/version" "$STAGE_ROOT/usr/local/share/vitte/VERSION"
if [ -f "$ROOT_DIR/NOTICE" ]; then
  install -m 0644 "$ROOT_DIR/NOTICE" "$STAGE_ROOT/usr/local/share/vitte/NOTICE"
else
  cat > "$STAGE_ROOT/usr/local/share/vitte/NOTICE" <<EOF
Vitte NOTICE

No standalone NOTICE file exists in this source snapshot.
Refer to /usr/local/share/vitte/LICENSE for license details.
Generated by make-macos-pkg.sh on $(date -u +%Y-%m-%dT%H:%M:%SZ).
EOF
  chmod 0644 "$STAGE_ROOT/usr/local/share/vitte/NOTICE"
fi

# Read-only snapshots for post-install audit.
find "$STAGE_ROOT/usr/local/share/vitte/tests/modules/contracts" -type d -exec chmod 0555 {} +
find "$STAGE_ROOT/usr/local/share/vitte/tests/modules/contracts" -type f -exec chmod 0444 {} +

# Checksums/signature bundle for universal binary.
if [ -x "$CHECKSUM_TARGET_BIN" ]; then
  CHECKSUM_SRC_BIN="$CHECKSUM_TARGET_BIN"
else
  CHECKSUM_SRC_BIN="$VITTE_BIN"
fi
install -m 0755 "$CHECKSUM_SRC_BIN" "$STAGE_ROOT/usr/local/share/vitte/checksums/vitte"
sha256_file "$CHECKSUM_SRC_BIN" > "$STAGE_ROOT/usr/local/share/vitte/checksums/vitte.sha256"
sha512_file "$CHECKSUM_SRC_BIN" > "$STAGE_ROOT/usr/local/share/vitte/checksums/vitte.sha512"
{
  echo "file: vitte"
  echo "source: $CHECKSUM_SRC_BIN"
  echo "generated_at_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  lipo -info "$CHECKSUM_SRC_BIN" 2>/dev/null || true
  codesign -dv --verbose=4 "$CHECKSUM_SRC_BIN" 2>&1 || echo "codesign: unavailable or unsigned"
} > "$STAGE_ROOT/usr/local/share/vitte/checksums/vitte.signature.txt"
chmod 0644 \
  "$STAGE_ROOT/usr/local/share/vitte/checksums/vitte.sha256" \
  "$STAGE_ROOT/usr/local/share/vitte/checksums/vitte.sha512" \
  "$STAGE_ROOT/usr/local/share/vitte/checksums/vitte.signature.txt"

# Uninstall helper.
cat > "$STAGE_ROOT/usr/local/share/vitte/uninstall.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

log() { printf "[vitte uninstall] %s\n" "$*"; }

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

  remove_path "$home_dir/.vim/syntax/vitte.vim"
  remove_path "$home_dir/.vim/indent/vitte.vim"
  remove_path "$home_dir/.vim/ftdetect/vitte.vim"
  remove_path "$home_dir/.vim/ftplugin/vitte.vim"
  remove_path "$home_dir/.vim/compiler/vitte.vim"
  remove_path "$home_dir/.emacs.d/lisp/vitte-mode.el"
  remove_path "$home_dir/.emacs.d/lisp/vitte-indent.el"
  remove_path "$home_dir/.config/nano/vitte.nanorc"
  remove_path "$home_dir/.config/geany/filedefs/filetypes.vitte.conf"
  remove_path "$home_dir/.config/geany/filedefs/snippets.vitte.conf"
  remove_path "$home_dir/.config/geany/filetype_extensions.conf"
  remove_path "$home_dir/Library/Application Support/geany/filedefs/filetypes.vitte.conf"
  remove_path "$home_dir/Library/Application Support/geany/filedefs/snippets.vitte.conf"
  remove_path "$home_dir/Library/Application Support/geany/filetype_extensions.conf"

  local nanorc="$home_dir/.nanorc"
  if [ -f "$nanorc" ]; then
    cp "$nanorc" "$nanorc.vitte-uninstall.bak"
    grep -v '^include "~/.config/nano/vitte.nanorc"$' "$nanorc.vitte-uninstall.bak" > "$nanorc.tmp" || true
    mv "$nanorc.tmp" "$nanorc"
    log "updated: $nanorc (backup: $nanorc.vitte-uninstall.bak)"
  fi

  for emacs_init in "$home_dir/.emacs.d/init.el" "$home_dir/.emacs"; do
    if [ -f "$emacs_init" ]; then
      cp "$emacs_init" "$emacs_init.vitte-uninstall.bak"
      grep -v '^;; vitte-mode setup (managed by vitte pkg)$' "$emacs_init.vitte-uninstall.bak" \
        | grep -v "^(add-to-list 'load-path \"~/.emacs.d/lisp\")$" \
        | grep -v "^(autoload 'vitte-mode \"vitte-mode\" nil t)$" \
        | grep -v "^(add-to-list 'auto-mode-alist '(\"\\\\.vit\\\\'\" . vitte-mode))$" \
        > "$emacs_init.tmp" || true
      mv "$emacs_init.tmp" "$emacs_init"
      log "updated: $emacs_init (backup: $emacs_init.vitte-uninstall.bak)"
    fi
  done
}

remove_path "/usr/local/bin/vitte"
remove_path "/usr/local/bin/vittec"
remove_path "/usr/local/bin/vitte-linker"
remove_path "/usr/local/bin/vitte-editor"
remove_path "/usr/local/libexec/vitte"
remove_path "/usr/local/etc/bash_completion.d/vitte"
remove_path "/usr/local/share/zsh/site-functions/_vitte"
remove_path "/usr/local/share/fish/vendor_completions.d/vitte.fish"
remove_path "/usr/local/share/man/man1/vitte.1"
remove_path "/usr/local/share/man/man1/vittec.1"
remove_path "/usr/local/share/man/man1/vitte-linker.1"
remove_path "/usr/local/share/man/man1/vitte-editor.1"
remove_path "/usr/local/share/vitte"

rmdir "/usr/local/share/man/man1" 2>/dev/null || true
rmdir "/usr/local/share/man" 2>/dev/null || true
rmdir "/usr/local/libexec" 2>/dev/null || true

if [ "${REMOVE_USER_EDITOR_CONFIG:-0}" = "1" ]; then
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

log "done"
EOF
chmod 0755 "$STAGE_ROOT/usr/local/share/vitte/uninstall.sh"

# Man pages.
mkdir -p "$STAGE_ROOT/usr/local/share/man/man1"
for m in vitte.1 vittec.1 vitte-linker.1 vitte-editor.1; do
  [ -f "$ROOT_DIR/man/$m" ] && install -m 0644 "$ROOT_DIR/man/$m" "$STAGE_ROOT/usr/local/share/man/man1/$m"
done

# Shell completions (full tree in share + installed in standard lookup paths).
mkdir -p "$STAGE_ROOT/usr/local/etc/bash_completion.d"
mkdir -p "$STAGE_ROOT/usr/local/share/zsh/site-functions"
mkdir -p "$STAGE_ROOT/usr/local/share/fish/vendor_completions.d"
rsync -a "$ROOT_DIR/completions/" "$STAGE_ROOT/usr/local/share/vitte/completions/"

install -m 0644 "$ROOT_DIR/completions/bash/vitte" "$STAGE_ROOT/usr/local/etc/bash_completion.d/vitte"
install -m 0644 "$ROOT_DIR/completions/zsh/_vitte" "$STAGE_ROOT/usr/local/share/zsh/site-functions/_vitte"
install -m 0644 "$ROOT_DIR/completions/fish/vitte.fish" "$STAGE_ROOT/usr/local/share/fish/vendor_completions.d/vitte.fish"

# Install manifest JSON (debian-control style metadata + all installed files).
python3 - "$STAGE_ROOT" "$IDENTIFIER" "$VERSION" > "$STAGE_ROOT/usr/local/share/vitte/manifest/pkg_manifest.json" <<'PY'
import json
import os
import platform
import sys
from datetime import datetime, timezone

stage = sys.argv[1]
identifier = sys.argv[2]
version = sys.argv[3]
files = []
for root, _, names in os.walk(stage):
    for n in names:
        abs_path = os.path.join(root, n)
        rel = os.path.relpath(abs_path, stage)
        files.append("/" + rel.replace(os.sep, "/"))
files.sort()
payload = {
    "package": "vitte",
    "identifier": identifier,
    "version": version,
    "format": "macos-pkg",
    "control_style": "debian-like",
    "architecture": platform.machine(),
    "generated_at_utc": datetime.now(timezone.utc).isoformat(),
    "files": files,
}
json.dump(payload, sys.stdout, indent=2)
sys.stdout.write("\n")
PY
chmod 0644 "$STAGE_ROOT/usr/local/share/vitte/manifest/pkg_manifest.json"

find "$STAGE_ROOT" -name '.DS_Store' -type f -delete || true

# Postinstall verification.
cat > "$SCRIPTS_DIR/postinstall" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

LOG_FILE="/var/log/vitte-install.log"
if ! touch "$LOG_FILE" >/dev/null 2>&1; then
  LOG_FILE="/tmp/vitte-install.log"
  touch "$LOG_FILE" >/dev/null 2>&1 || true
fi

log_line() {
  printf "%s %s\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" >> "$LOG_FILE" 2>/dev/null || true
}

install_user_editor_support() {
  local user_name="$1"
  local home_dir="$2"
  local editor_root="/usr/local/share/vitte/editors"
  [ -n "$user_name" ] || return 0
  [ -d "$home_dir" ] || return 0
  [ -d "$editor_root" ] || return 0

  # Vim syntax/ftplugin/compiler.
  mkdir -p "$home_dir/.vim/syntax" "$home_dir/.vim/indent" "$home_dir/.vim/ftdetect" "$home_dir/.vim/ftplugin" "$home_dir/.vim/compiler"
  install -m 0644 "$editor_root/vim/syntax/vitte.vim" "$home_dir/.vim/syntax/vitte.vim"
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

  # Geany configuration (filedefs + snippets + *.vit mapping).
  if [ -x "$editor_root/geany/install_geany.sh" ]; then
    HOME="$home_dir" \
    XDG_CONFIG_HOME="$home_dir/.config" \
    APPDATA="$home_dir/AppData/Roaming" \
    VITTE_GEANY_WD_MODE="${VITTE_GEANY_WD_MODE:-file}" \
    "$editor_root/geany/install_geany.sh" >/dev/null 2>&1 || true
  fi

  chown -R "$user_name" "$home_dir/.vim" "$home_dir/.emacs.d" "$home_dir/.config/nano" "$home_dir/.config/geany" "$home_dir/.nanorc" "$home_dir/Library/Application Support/geany" >/dev/null 2>&1 || true
}

if ! /usr/local/bin/vitte --help >/dev/null 2>&1; then
  log_line "[error] vitte --help failed"
  echo "[vitte pkg] postinstall check failed: vitte --help" >&2
  exit 1
fi
log_line "[ok] vitte --help"

TMP="/tmp/vitte_pkg_verify_$$.vit"
cat > "$TMP" <<'VEOF'
use vitte/core as core_pkg

entry main at core/app {
  give 0
}
VEOF

if ! /usr/local/bin/vitte check "$TMP" >/dev/null 2>&1; then
  log_line "[warn] vitte check package import test failed"
  echo "[vitte pkg] warning: package import test failed (see $LOG_FILE)" >&2
  rm -f "$TMP"
fi
rm -f "$TMP"
log_line "[ok] vitte check package import test (or warning already logged)"

for p in \
  /usr/local/etc/bash_completion.d/vitte \
  /usr/local/share/zsh/site-functions/_vitte \
  /usr/local/share/fish/vendor_completions.d/vitte.fish
do
  if [ ! -f "$p" ]; then
    log_line "[error] missing completion $p"
    echo "[vitte pkg] postinstall check failed: missing completion $p" >&2
    exit 1
  fi
done
log_line "[ok] completions found"

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
log_line "[ok] editor support installed"

if /usr/local/bin/vitte doctor --help >/dev/null 2>&1; then
  log_line "[run] vitte doctor"
  /usr/local/bin/vitte doctor >> "$LOG_FILE" 2>&1 || log_line "[warn] vitte doctor returned non-zero"
else
  log_line "[info] vitte doctor command unavailable"
fi

cat <<MSG
[vitte pkg] install complete
[vitte pkg] binary: /usr/local/bin/vitte
[vitte pkg] root:   /usr/local/share/vitte
[vitte pkg] man:    /usr/local/share/man/man1/{vitte,vittec,vitte-linker,vitte-editor}.1
[vitte pkg] editors: ~/.vim ~/.emacs.d ~/.config/nano + Geany configured
[vitte pkg] zsh:    autoload -U compinit && compinit
[vitte pkg] env:    source /usr/local/share/vitte/env.sh
[vitte pkg] log:    $LOG_FILE
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
