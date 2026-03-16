#!/usr/bin/env bash
# ============================================================
# vitte — Debian .deb builder (parity with macOS pkg payload)
# Includes (profile=full): binaries, std/packages, runtime sources, editors,
# manpages, shell completions, env helper, postinst checks.
# ============================================================

set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
DEPS_FILE="${DEPS_FILE:-$ROOT_DIR/toolchain/scripts/install/debian-deps.sh}"

VERSION="${VERSION:-2.1.1}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"
MAINTAINER="${MAINTAINER:-Vitte Team <maintainers@vitte.dev>}"
DESCRIPTION="${DESCRIPTION:-Vitte systems language toolchain}"
HOMEPAGE="${HOMEPAGE:-https://vitte-lang.org/}"
VCS_URL="${VCS_URL:-https://github.com/vitte/vitte}"
BUGS_URL="${BUGS_URL:-https://github.com/vitte/vitte/issues}"
SECTION="${SECTION:-devel}"
PRIORITY="${PRIORITY:-optional}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"
ARCH="${ARCH:-$(dpkg --print-architecture 2>/dev/null || echo amd64)}"
DEPENDS="${DEPENDS:-}"
RECOMMENDS="${RECOMMENDS:-}"
PROJECT_INFO_TITLE="${PROJECT_INFO_TITLE:-Vitte systems language toolchain}"
PROJECT_LICENSE="${PROJECT_LICENSE:-}"
RELEASE_READINESS_GATE="${RELEASE_READINESS_GATE:-1}"
PACKAGE_PROFILE="${PACKAGE_PROFILE:-full}"
SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-}"
PACKAGE_AUDIT="${PACKAGE_AUDIT:-1}"

STAGE_BASE="$ROOT_DIR/.debstage/${PACKAGE_NAME}-${VERSION}"
STAGE_ROOT="$STAGE_BASE/root"
DEBIAN_DIR="$STAGE_ROOT/DEBIAN"
OUT_DEB="$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"

log() { printf "[make-debian-deb] %s\n" "$*"; }
die() { printf "[make-debian-deb][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

detect_project_license() {
  local license_file="$ROOT_DIR/LICENSE"
  [ -f "$license_file" ] || { printf "Custom\n"; return 0; }
  if grep -Eqi 'GNU (GENERAL )?PUBLIC LICENSE|GPL[^[:alnum:]]*v?3(\.0)?' "$license_file"; then
    printf "GPL-3.0-only\n"
    return 0
  fi
  printf "Custom\n"
}

dep5_license_name() {
  case "${1:-}" in
    GPL-3.0-only) printf "GPL-3\n" ;;
    *) printf "%s\n" "${1:-Custom}" ;;
  esac
}

setup_profile_flags() {
  case "$PACKAGE_PROFILE" in
    full)
      INCLUDE_EDITORS=1
      INCLUDE_COMPLETIONS=1
      INCLUDE_MANPAGES=1
      INCLUDE_DOCS=1
      INCLUDE_ICONS=1
      INCLUDE_APPSTREAM=1
      ;;
    minimal)
      INCLUDE_EDITORS=0
      INCLUDE_COMPLETIONS=0
      INCLUDE_MANPAGES=1
      INCLUDE_DOCS=1
      INCLUDE_ICONS=0
      INCLUDE_APPSTREAM=1
      ;;
    *)
      die "unknown PACKAGE_PROFILE=$PACKAGE_PROFILE (supported: full|minimal)"
      ;;
  esac
}

if [ ! -f "$DEPS_FILE" ]; then
  die "missing deps file: $DEPS_FILE"
fi
# shellcheck disable=SC1090
source "$DEPS_FILE"
setup_profile_flags
if [ -z "$DEPENDS" ]; then
  DEPENDS="${DEBIAN_STRICT_RUNTIME_DEPENDS:-$DEBIAN_RUNTIME_DEPENDS}"
fi
if [ -z "$RECOMMENDS" ]; then
  RECOMMENDS="${DEBIAN_RUNTIME_RECOMMENDS:-}"
fi
if [ -z "$PROJECT_LICENSE" ]; then
  PROJECT_LICENSE="$(detect_project_license)"
fi
DEP5_LICENSE="$(dep5_license_name "$PROJECT_LICENSE")"
if [ -z "$SOURCE_DATE_EPOCH" ]; then
  SOURCE_DATE_EPOCH="$(git -C "$ROOT_DIR" log -1 --format=%ct 2>/dev/null || true)"
fi
if [ -z "$SOURCE_DATE_EPOCH" ]; then
  SOURCE_DATE_EPOCH="$(date -u +%s)"
fi
export SOURCE_DATE_EPOCH TZ=UTC LC_ALL=C

RSYNC_COMMON_EXCLUDES=(
  --exclude '.git/'
  --exclude '.vscode-test/'
  --exclude 'node_modules/'
  --exclude '__pycache__/'
  --exclude 'tests/'
  --exclude 'test/'
  --exclude '.DS_Store'
)

resolve_bin() {
  local name="$1"
  local a="$ROOT_DIR/bin/$name"
  local b="$ROOT_DIR/target/bin/$name"
  if [ -x "$a" ]; then printf "%s\n" "$a"; return 0; fi
  if [ -x "$b" ]; then printf "%s\n" "$b"; return 0; fi
  return 1
}

ensure_bin() {
  local name="$1"
  local make_target="$2"
  local src
  src="$(resolve_bin "$name" || true)"
  if [ -n "$src" ]; then
    printf "%s\n" "$src"
    return 0
  fi
  if has make; then
    log "building missing binary '$name' via: make $make_target"
    make -C "$ROOT_DIR" "$make_target"
    src="$(resolve_bin "$name" || true)"
  fi
  [ -n "$src" ] || die "$name binary not found (expected bin/$name or target/bin/$name)"
  printf "%s\n" "$src"
}

require_tools() {
  has dpkg-deb || die "dpkg-deb is required"
  has rsync || die "rsync is required"
  has python3 || die "python3 is required"
}

normalize_stage_timestamps() {
  local epoch="$SOURCE_DATE_EPOCH"
  find "$STAGE_ROOT" -print0 | xargs -0 touch -h -d "@$epoch"
}

run_release_readiness_gate() {
  [ "$RELEASE_READINESS_GATE" = "1" ] || {
    log "release-readiness gate disabled (RELEASE_READINESS_GATE=$RELEASE_READINESS_GATE)"
    return 0
  }
  has make || die "make is required for release-readiness gate"
  log "running release-readiness gate before Debian packaging"
  make -C "$ROOT_DIR" -s release-readiness
}

stage_payload() {
  local vitte_bin
  vitte_bin="$(ensure_bin vitte build)"

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

  mkdir -p "$STAGE_ROOT/usr/local/share/vitte/src/vitte"
  mkdir -p "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends"

  rsync -a "${RSYNC_COMMON_EXCLUDES[@]}" "$ROOT_DIR/src/vitte/packages/" "$STAGE_ROOT/usr/local/share/vitte/src/vitte/packages/"
  rsync -a "${RSYNC_COMMON_EXCLUDES[@]}" "$ROOT_DIR/src/compiler/backends/runtime/" "$STAGE_ROOT/usr/local/share/vitte/src/compiler/backends/runtime/"
  if [ -f "$ROOT_DIR/version" ]; then
    install -m 0644 "$ROOT_DIR/version" "$STAGE_ROOT/usr/local/share/vitte/version"
  fi

  if [ "$INCLUDE_EDITORS" = "1" ]; then
    mkdir -p "$STAGE_ROOT/usr/local/share/vitte/editors"
    if [ -d "$ROOT_DIR/editors" ]; then
      rsync -a \
        "${RSYNC_COMMON_EXCLUDES[@]}" \
        --exclude 'vscode/' \
        "$ROOT_DIR/editors/" "$STAGE_ROOT/usr/local/share/vitte/editors/"
    fi
    [ -f "$ROOT_DIR/editors/README.md" ] && install -m 0644 "$ROOT_DIR/editors/README.md" "$STAGE_ROOT/usr/local/share/vitte/editors/README.md"
  fi

  install -m 0644 "$ROOT_DIR/toolchain/scripts/install/templates/env.sh" "$STAGE_ROOT/usr/local/share/vitte/env.sh"

  if [ "$INCLUDE_MANPAGES" = "1" ]; then
    mkdir -p "$STAGE_ROOT/usr/local/share/man/man1"
    local manpages="vitte.1"
    for m in $manpages; do
      [ -f "$ROOT_DIR/man/$m" ] && install -m 0644 "$ROOT_DIR/man/$m" "$STAGE_ROOT/usr/local/share/man/man1/$m"
    done
  fi

  if [ "$INCLUDE_COMPLETIONS" = "1" ]; then
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
  fi

  mkdir -p "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME"
  [ -f "$ROOT_DIR/LICENSE" ] && install -m 0644 "$ROOT_DIR/LICENSE" "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/LICENSE"
  if [ "$INCLUDE_DOCS" = "1" ]; then
    [ -f "$ROOT_DIR/README.md" ] && install -m 0644 "$ROOT_DIR/README.md" "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/README.md"
    [ -f "$ROOT_DIR/CHANGELOG.md" ] && install -m 0644 "$ROOT_DIR/CHANGELOG.md" "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/CHANGELOG.md"
    [ -f "$ROOT_DIR/CONTRIBUTING.md" ] && install -m 0644 "$ROOT_DIR/CONTRIBUTING.md" "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/CONTRIBUTING.md"
    [ -f "$ROOT_DIR/CODE_OF_CONDUCT.md" ] && install -m 0644 "$ROOT_DIR/CODE_OF_CONDUCT.md" "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/CODE_OF_CONDUCT.md"
    [ -f "$ROOT_DIR/SECURITY.md" ] && install -m 0644 "$ROOT_DIR/SECURITY.md" "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/SECURITY.md"
    [ -f "$ROOT_DIR/SUPPORT.md" ] && install -m 0644 "$ROOT_DIR/SUPPORT.md" "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/SUPPORT.md"
  fi

  local git_commit
  git_commit="$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || true)"
  cat > "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/PROJECT_INFO" <<EOF
Name: ${PACKAGE_NAME}
Title: ${PROJECT_INFO_TITLE}
Description: ${DESCRIPTION}
Version: ${VERSION}
Architecture: ${ARCH}
Homepage: ${HOMEPAGE}
VCS: ${VCS_URL}
Issues: ${BUGS_URL}
Maintainer: ${MAINTAINER}
License: ${PROJECT_LICENSE}
Package-Profile: ${PACKAGE_PROFILE}
Build-Date-UTC: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
Git-Commit: ${git_commit:-unknown}
Binary: /usr/local/bin/vitte
Root: /usr/local/share/vitte
License-File: /usr/share/doc/${PACKAGE_NAME}/LICENSE
EOF

  if [ "$INCLUDE_ICONS" = "1" ]; then
    mkdir -p "$STAGE_ROOT/usr/share/icons/hicolor/scalable/apps"
    mkdir -p "$STAGE_ROOT/usr/share/pixmaps"
    install -m 0644 "$ROOT_DIR/toolchain/assets/vitte-logo-circle-blue.svg" "$STAGE_ROOT/usr/share/icons/hicolor/scalable/apps/vitte.svg"
    install -m 0644 "$ROOT_DIR/toolchain/assets/vitte-logo-circle-blue.svg" "$STAGE_ROOT/usr/share/pixmaps/vitte.svg"
  fi
  cat > "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/copyright" <<EOF
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: vitte
Upstream-Contact: Vitte Team <maintainers@vitte.dev>
Source: https://github.com/vitte/vitte

Files: *
Copyright: 2021-2026 Vitte contributors
License: ${DEP5_LICENSE}
EOF
  if [ "$PROJECT_LICENSE" = "GPL-3.0-only" ]; then
    cat >> "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/copyright" <<'EOF'
 This package is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 3.
 .
 This package is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 .
 On Debian systems, the full text of the GNU General Public License
 version 3 can be found in /usr/share/common-licenses/GPL-3.
EOF
  else
    cat >> "$STAGE_ROOT/usr/share/doc/$PACKAGE_NAME/copyright" <<'EOF'
 See /usr/share/doc/vitte/LICENSE for the full upstream license text.
EOF
  fi

  if [ "$INCLUDE_APPSTREAM" = "1" ]; then
    mkdir -p "$STAGE_ROOT/usr/share/metainfo"
    cat > "$STAGE_ROOT/usr/share/metainfo/vitte.metainfo.xml" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<component type="console-application">
  <id>org.vitte.lang</id>
  <name>Vitte</name>
  <summary>Vitte systems language toolchain</summary>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>${PROJECT_LICENSE}</project_license>
  <description>
    <p>Compiler and CLI runtime for the Vitte systems language.</p>
  </description>
  <url type="homepage">${HOMEPAGE}</url>
  <url type="bugtracker">${BUGS_URL}</url>
  <url type="vcs-browser">${VCS_URL}</url>
  <provides>
    <binary>vitte</binary>
  </provides>
  <releases>
    <release version="${VERSION}" date="$(date -u +"%Y-%m-%d")"/>
  </releases>
</component>
EOF
  fi

}

write_control_files() {
  mkdir -p "$DEBIAN_DIR"
  local installed_size
  local recommends_line
  installed_size="$(du -sk "$STAGE_ROOT" | awk '{print $1}')"
  recommends_line=""
  if [ -n "$RECOMMENDS" ]; then
    recommends_line="Recommends: ${RECOMMENDS}"
  fi

  cat > "$DEBIAN_DIR/control" <<EOF
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: ${SECTION}
Priority: ${PRIORITY}
Architecture: ${ARCH}
Maintainer: ${MAINTAINER}
Depends: ${DEPENDS}
${recommends_line}
Homepage: ${HOMEPAGE}
Installed-Size: ${installed_size}
Vcs-Browser: ${VCS_URL}
Vcs-Git: ${VCS_URL}.git
Bugs: ${BUGS_URL}
Description: ${DESCRIPTION}
 Unified Vitte toolchain package including binary, runtime sources,
 standard packages, profile-selected extras and packaging metadata.
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

  # Install Geany support using the upstream helper in user context.
  if [ -f "$editor_root/geany/install_geany.sh" ]; then
    if command -v runuser >/dev/null 2>&1; then
      runuser -u "$user_name" -- env HOME="$home_dir" \
        VITTE_GEANY_WD_MODE="${VITTE_GEANY_WD_MODE:-file}" \
        bash "$editor_root/geany/install_geany.sh" >/dev/null 2>&1 || true
    elif command -v su >/dev/null 2>&1; then
      su - "$user_name" -c "HOME='$home_dir' VITTE_GEANY_WD_MODE='${VITTE_GEANY_WD_MODE:-file}' bash '$editor_root/geany/install_geany.sh'" >/dev/null 2>&1 || true
    fi
  fi

  chown -R "$user_name" "$home_dir/.vim" "$home_dir/.emacs.d" "$home_dir/.config/nano" "$home_dir/.nanorc" >/dev/null 2>&1 || true
}

if [ "${1:-}" = "configure" ]; then
  if ! /usr/local/bin/vitte --help >/dev/null 2>&1; then
    echo "[vitte deb] postinst check failed: vitte --help" >&2
    exit 1
  fi
EOF
  cat >> "$DEBIAN_DIR/postinst" <<'EOF'

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

EOF
  if [ "$INCLUDE_COMPLETIONS" = "1" ]; then
    cat >> "$DEBIAN_DIR/postinst" <<'EOF'
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
EOF
  fi
  cat >> "$DEBIAN_DIR/postinst" <<'EOF'

  if [ -n "${VITTE_SETUP_USER:-}" ]; then
    install_user_editor_support "$VITTE_SETUP_USER"
  elif [ -n "${SUDO_USER:-}" ]; then
    install_user_editor_support "$SUDO_USER"
  fi

  cat <<MSG
[vitte deb] install complete
[vitte deb] description: see /usr/share/doc/vitte/PROJECT_INFO
[vitte deb] license: see /usr/share/doc/vitte/LICENSE
[vitte deb] profile: see /usr/share/doc/vitte/PROJECT_INFO
[vitte deb] installs: CLI, stdlib packages, runtime sources, profile extras
[vitte deb] binary: /usr/local/bin/vitte
EOF
  cat >> "$DEBIAN_DIR/postinst" <<'EOF'
[vitte deb] root:   /usr/local/share/vitte
[vitte deb] docs:   /usr/share/doc/vitte
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
  run_release_readiness_gate
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
  normalize_stage_timestamps

  if dpkg-deb --help 2>/dev/null | grep -q -- '--root-owner-group'; then
    dpkg-deb --build --root-owner-group "$STAGE_ROOT" "$OUT_DEB"
  else
    dpkg-deb --build "$STAGE_ROOT" "$OUT_DEB"
  fi

  log "wrote $OUT_DEB"
  ls -lh "$OUT_DEB"
  if [ "$PACKAGE_AUDIT" = "1" ] && [ -x "$ROOT_DIR/toolchain/scripts/package/audit-debian-deb.sh" ]; then
    "$ROOT_DIR/toolchain/scripts/package/audit-debian-deb.sh" "$OUT_DEB"
  fi
}

main "$@"
