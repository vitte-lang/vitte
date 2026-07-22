#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=build-linux-debs
. "$ROOT_DIR/scripts_build/common.sh"
scripts_build_parse_common_flags "$@"
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
OUT_DIR=${OUT_DIR:-$ROOT_DIR/pkgout}
case "$OUT_DIR" in /*) ;; *) OUT_DIR=$ROOT_DIR/$OUT_DIR ;; esac
ARCH=${ARCH:-all}
PACKAGE_NAME=${PACKAGE_NAME:-vitte}
SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git -C "$ROOT_DIR" log -1 --format=%ct 2>/dev/null || date +%s)}

EDITORS_DIR=$ROOT_DIR/editors
COMPLETIONS_DIR=$ROOT_DIR/completions
LICENSE_FILE=$ROOT_DIR/LICENSE
LOGO_FILE=$ROOT_DIR/assets/logo.png

PACKAGE_VERSION_FILE=$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION
PAYLOAD_SCRIPT=$ROOT_DIR/scripts_build/stage-installer-payload.sh
scripts_build_maybe_help "usage: build-linux-debs.sh [--dry-run]"
scripts_build_maybe_dry_run "would build Linux deb packages version=$VERSION arch=$ARCH out=$OUT_DIR"

die() {
  printf '[build-linux-debs][error] %s\n' "$*" >&2
  exit 1
}

require() {
  command -v "$1" >/dev/null 2>&1 ||
    die "missing required tool: $1"
}

require_file() {
  file=$1
  description=$2

  [ -f "$file" ] ||
    die "$description not found: $file"

  [ -s "$file" ] ||
    die "$description is empty: $file"
}

require_directory() {
  directory=$1
  description=$2

  [ -d "$directory" ] ||
    die "$description directory not found: $directory"

  find "$directory" -type f -print -quit |
    grep -q . ||
    die "$description directory is empty: $directory"
}

find_first_file() {
  description=$1
  shift

  for candidate in "$@"; do
    if [ -f "$candidate" ] && [ -s "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  die "$description not found; checked: $*"
}

copy_tree() {
  source_dir=$1
  destination_dir=$2
  description=$3

  require_directory "$source_dir" "$description"

  mkdir -p "$destination_dir"

  cp -R "$source_dir"/. "$destination_dir"/

  printf '[build-linux-debs] added %s: %s\n' \
    "$description" \
    "$destination_dir"
}

verify_license_file() {
  license_file=$1

  require_file "$license_file" "GNU GPLv3 license"

  grep -F 'GNU GENERAL PUBLIC LICENSE' "$license_file" >/dev/null ||
    die "invalid GNU GPL license file: missing license title"

  grep -F 'Version 3, 29 June 2007' "$license_file" >/dev/null ||
    die "invalid GNU GPL license file: expected GPL version 3"
}

add_archived_integrations() {
  data_root=$1
  share_root=$data_root/usr/local/share/vitte

  copy_tree \
    "$EDITORS_DIR" \
    "$share_root/editors" \
    "archived editor integrations"

  copy_tree \
    "$COMPLETIONS_DIR" \
    "$share_root/completions" \
    "shell completions"
}

install_vim_integration() {
  data_root=$1

  vim_syntax=$(find_first_file \
    "Vim Vitte syntax file" \
    "$EDITORS_DIR/vim/syntax/vitte.vim" \
    "$EDITORS_DIR/vim/vitte.vim" \
    "$EDITORS_DIR/vitte.vim")

  local_vim_root=$data_root/usr/local/share/vim/vimfiles
  system_vim_root=$data_root/usr/share/vim/vimfiles

  for vim_root in \
    "$local_vim_root" \
    "$system_vim_root"
  do
    mkdir -p \
      "$vim_root/syntax" \
      "$vim_root/ftdetect" \
      "$vim_root/ftplugin"

    install -m 0644 \
      "$vim_syntax" \
      "$vim_root/syntax/vitte.vim"

    if [ -f "$EDITORS_DIR/vim/ftdetect/vitte.vim" ]; then
      install -m 0644 \
        "$EDITORS_DIR/vim/ftdetect/vitte.vim" \
        "$vim_root/ftdetect/vitte.vim"
    else
      cat > "$vim_root/ftdetect/vitte.vim" <<'EOF'
augroup vitte_filetype_detection
  autocmd!
  autocmd BufRead,BufNewFile *.vit setfiletype vitte
  autocmd BufRead,BufNewFile *.vitte setfiletype vitte
  autocmd BufRead,BufNewFile *.vitl setfiletype vitte
augroup END
EOF

      chmod 0644 "$vim_root/ftdetect/vitte.vim"
    fi

    if [ -f "$EDITORS_DIR/vim/ftplugin/vitte.vim" ]; then
      install -m 0644 \
        "$EDITORS_DIR/vim/ftplugin/vitte.vim" \
        "$vim_root/ftplugin/vitte.vim"
    else
      cat > "$vim_root/ftplugin/vitte.vim" <<'EOF'
if exists("b:did_ftplugin")
  finish
endif

let b:did_ftplugin = 1

setlocal commentstring=//\ %s
setlocal comments=s1:/*,mb:*,ex:*/,://
setlocal expandtab
setlocal shiftwidth=4
setlocal softtabstop=4
setlocal tabstop=4
EOF

      chmod 0644 "$vim_root/ftplugin/vitte.vim"
    fi
  done

  printf '[build-linux-debs] installed Vim syntax integration\n'
}

install_emacs_integration() {
  data_root=$1

  emacs_mode=$(find_first_file \
    "Emacs Vitte mode" \
    "$EDITORS_DIR/emacs/vitte-mode.el" \
    "$EDITORS_DIR/emacs/vitte.el" \
    "$EDITORS_DIR/vitte-mode.el")

  local_emacs_root=$data_root/usr/local/share/emacs/site-lisp
  system_emacs_root=$data_root/usr/share/emacs/site-lisp/vitte
  startup_root=$data_root/etc/emacs/site-start.d

  mkdir -p \
    "$local_emacs_root" \
    "$system_emacs_root" \
    "$startup_root"

  install -m 0644 \
    "$emacs_mode" \
    "$local_emacs_root/vitte-mode.el"

  install -m 0644 \
    "$emacs_mode" \
    "$system_emacs_root/vitte-mode.el"

  cat > "$startup_root/50vitte.el" <<'EOF'
;;; 50vitte.el --- system-wide Vitte mode registration

(add-to-list 'load-path "/usr/local/share/emacs/site-lisp")
(add-to-list 'load-path "/usr/share/emacs/site-lisp/vitte")

(autoload 'vitte-mode
  "vitte-mode"
  "Major mode for the Vitte programming language."
  t)

(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
(add-to-list 'auto-mode-alist '("\\.vitte\\'" . vitte-mode))
(add-to-list 'auto-mode-alist '("\\.vitl\\'" . vitte-mode))

(provide 'vitte-system-init)

;;; 50vitte.el ends here
EOF

  chmod 0644 "$startup_root/50vitte.el"

  printf '[build-linux-debs] installed Emacs mode integration\n'
}

install_nano_integration() {
  data_root=$1

  nano_syntax=$(find_first_file \
    "Nano Vitte syntax file" \
    "$EDITORS_DIR/nano/vitte.nanorc" \
    "$EDITORS_DIR/nano/vitte.nano" \
    "$EDITORS_DIR/vitte.nanorc")

  system_nano_root=$data_root/usr/share/nano
  local_nano_root=$data_root/usr/local/share/nano

  mkdir -p \
    "$system_nano_root" \
    "$local_nano_root"

  install -m 0644 \
    "$nano_syntax" \
    "$system_nano_root/vitte.nanorc"

  install -m 0644 \
    "$nano_syntax" \
    "$local_nano_root/vitte.nanorc"

  printf '[build-linux-debs] installed Nano syntax integration\n'
}

install_geany_integration() {
  data_root=$1

  geany_definition=$(find_first_file \
    "Geany Vitte filetype definition" \
    "$EDITORS_DIR/geany/filetypes.Vitte.conf" \
    "$EDITORS_DIR/geany/filetypes.vitte.conf" \
    "$EDITORS_DIR/geany/vitte.conf" \
    "$EDITORS_DIR/filetypes.Vitte.conf")

  system_geany_root=$data_root/usr/share/geany/filedefs
  local_geany_root=$data_root/usr/local/share/geany/filedefs

  mkdir -p \
    "$system_geany_root" \
    "$local_geany_root"

  install -m 0644 \
    "$geany_definition" \
    "$system_geany_root/filetypes.Vitte.conf"

  install -m 0644 \
    "$geany_definition" \
    "$local_geany_root/filetypes.Vitte.conf"

  printf '[build-linux-debs] installed Geany filetype integration\n'
}

install_editor_integrations() {
  data_root=$1

  add_archived_integrations "$data_root"
  install_vim_integration "$data_root"
  install_emacs_integration "$data_root"
  install_nano_integration "$data_root"
  install_geany_integration "$data_root"
}

add_license_and_logo() {
  data_root=$1

  license_destination=$data_root/usr/local/share/licenses/$PACKAGE_NAME
  debian_license_destination=$data_root/usr/share/doc/$PACKAGE_NAME
  assets_destination=$data_root/usr/local/share/vitte/assets

  mkdir -p \
    "$license_destination" \
    "$debian_license_destination" \
    "$assets_destination"

  install -m 0644 \
    "$LICENSE_FILE" \
    "$license_destination/LICENSE"

  install -m 0644 \
    "$LICENSE_FILE" \
    "$debian_license_destination/copyright"

  install -m 0644 \
    "$LOGO_FILE" \
    "$assets_destination/logo.png"

  printf '[build-linux-debs] added GNU GPLv3 license: %s\n' \
    "$license_destination/LICENSE"

  printf '[build-linux-debs] added Debian copyright file: %s\n' \
    "$debian_license_destination/copyright"

  printf '[build-linux-debs] added Vitte logo: %s\n' \
    "$assets_destination/logo.png"
}

verify_directory_not_empty() {
  directory=$1
  description=$2

  [ -d "$directory" ] ||
    die "$description directory missing: $directory"

  find "$directory" -type f -print -quit |
    grep -q . ||
    die "$description directory is empty: $directory"
}

verify_payload() {
  data_root=$1
  arch=$2

  [ -x "$data_root/usr/local/bin/vitte" ] ||
    die "missing or non-executable Vitte command for Linux $arch"

  verify_directory_not_empty \
    "$data_root/usr/local/share/vitte/editors" \
    "archived editor integrations"

  verify_directory_not_empty \
    "$data_root/usr/local/share/vitte/completions" \
    "shell completions"

  require_file \
    "$data_root/usr/share/vim/vimfiles/syntax/vitte.vim" \
    "Vim Vitte syntax"

  require_file \
    "$data_root/usr/share/vim/vimfiles/ftdetect/vitte.vim" \
    "Vim Vitte filetype detection"

  require_file \
    "$data_root/usr/share/vim/vimfiles/ftplugin/vitte.vim" \
    "Vim Vitte filetype plugin"

  require_file \
    "$data_root/usr/share/emacs/site-lisp/vitte/vitte-mode.el" \
    "Emacs Vitte mode"

  require_file \
    "$data_root/etc/emacs/site-start.d/50vitte.el" \
    "Emacs Vitte automatic loader"

  require_file \
    "$data_root/usr/share/nano/vitte.nanorc" \
    "Nano Vitte syntax"

  require_file \
    "$data_root/usr/share/geany/filedefs/filetypes.Vitte.conf" \
    "Geany Vitte filetype"

  verify_license_file \
    "$data_root/usr/local/share/licenses/$PACKAGE_NAME/LICENSE"

  require_file \
    "$data_root/usr/share/doc/$PACKAGE_NAME/copyright" \
    "Debian package copyright file"

  require_file \
    "$data_root/usr/local/share/vitte/assets/logo.png" \
    "Vitte logo"
}

normalize_payload() {
  data_root=$1

  find "$data_root" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} + \
    2>/dev/null ||
    find "$data_root" -exec touch -h -t \
      "$(date -u -r "$SOURCE_DATE_EPOCH" '+%Y%m%d%H%M.%S')" {} +
}

generate_md5sums() {
  data_root=$1
  output_file=$2

  python3 - "$data_root" "$output_file" <<'PY'
import hashlib
import sys
from pathlib import Path

root = Path(sys.argv[1])
output = Path(sys.argv[2])

lines: list[str] = []

for path in sorted(root.rglob("*")):
    if not path.is_file() or path.is_symlink():
        continue

    digest = hashlib.md5(path.read_bytes()).hexdigest()
    relative = path.relative_to(root).as_posix()
    lines.append(f"{digest}  {relative}")

output.write_text(
    "\n".join(lines) + ("\n" if lines else ""),
    encoding="utf-8",
)
PY
}

write_control_file() {
  control_root=$1
  arch=$2
  installed_size=$3

  cat > "$control_root/control" <<EOF
Package: $PACKAGE_NAME
Version: $VERSION
Section: devel
Priority: optional
Architecture: $arch
Maintainer: Vitte Team <maintainers@vitte-lang.org>
Depends: bash, python3, make
Recommends: git, clang | gcc
Suggests: vim | neovim, emacs, nano, geany
Provides: vitte-compiler, vitte-toolchain, vitte-editor-support
Installed-Size: $installed_size
Homepage: https://vitte-lang.org/
Vcs-Browser: https://github.com/vitte-lang/vitte
X-Vitte-Processor: $arch
Description: Complete Vitte systems language toolchain
 Compiler, runtime, standard library, sources, documentation, examples,
 shell completions, GNU GPLv3 license and syntax highlighting for Vim,
 Neovim, Emacs, Nano and Geany.
EOF
}

write_maintainer_scripts() {
  control_root=$1

  cat > "$control_root/postinst" <<'EOF'
#!/bin/sh
set -eu

ensure_executable() {
  command_path=$1

  if [ -f "$command_path" ]; then
    chmod 0755 "$command_path"
  fi
}

ensure_nano_include() {
  nanorc=$1
  syntax_file=$2

  [ -f "$syntax_file" ] || return 0

  include_line="include \"$syntax_file\""

  if [ -f "$nanorc" ]; then
    if ! grep -F "$include_line" "$nanorc" >/dev/null 2>&1; then
      printf '\n%s\n' "$include_line" >> "$nanorc"
    fi
  else
    mkdir -p "$(dirname "$nanorc")"
    printf '%s\n' "$include_line" > "$nanorc"
  fi
}

ensure_executable /usr/local/bin/vitte
ensure_executable /usr/local/bin/vittec
ensure_executable /usr/local/bin/vittec0
ensure_executable /usr/local/libexec/vitte/vitte
ensure_executable /usr/local/libexec/vitte/vittec
ensure_executable /usr/local/libexec/vitte/vittec0

ensure_nano_include \
  /etc/nanorc \
  /usr/share/nano/vitte.nanorc

if command -v update-alternatives >/dev/null 2>&1; then
  :
fi

if command -v update-icon-caches >/dev/null 2>&1; then
  update-icon-caches /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database >/dev/null 2>&1 || true
fi

printf '%s\n' 'Vitte editor integrations installed:'
printf '%s\n' '  Vim/Neovim: *.vit, *.vitte, *.vitl'
printf '%s\n' '  Emacs:      *.vit, *.vitte, *.vitl'
printf '%s\n' '  Nano:       *.vit, *.vitte, *.vitl'
printf '%s\n' '  Geany:      Vitte filetype definition'

exit 0
EOF

  chmod 0755 "$control_root/postinst"

  cat > "$control_root/prerm" <<'EOF'
#!/bin/sh
set -eu

exit 0
EOF

  chmod 0755 "$control_root/prerm"

  cat > "$control_root/postrm" <<'EOF'
#!/bin/sh
set -eu

remove_nano_include() {
  nanorc=$1
  syntax_file=$2

  [ -f "$nanorc" ] || return 0

  include_line="include \"$syntax_file\""
  temporary_file="${nanorc}.vitte-tmp.$$"

  grep -Fv "$include_line" "$nanorc" > "$temporary_file" || true
  cat "$temporary_file" > "$nanorc"
  rm -f "$temporary_file"
}

case "${1:-}" in
  purge)
    remove_nano_include \
      /etc/nanorc \
      /usr/share/nano/vitte.nanorc
    ;;
esac

exit 0
EOF

  chmod 0755 "$control_root/postrm"
}

create_tar_archive() {
  archive=$1
  source_directory=$2

  COPYFILE_DISABLE=1 \
    bsdtar \
      --uid 0 \
      --gid 0 \
      --uname root \
      --gname root \
      --options gzip:timestamp \
      -czf "$archive" \
      -C "$source_directory" \
      .
}

verify_debian_package() {
  package_file=$1

  listing_file=$(mktemp)
  control_listing=$(mktemp)
  data_listing=$(mktemp)
  temporary_directory=$(mktemp -d)

  trap '
    rm -rf "$temporary_directory"
    rm -f "$listing_file" "$control_listing" "$data_listing"
  ' EXIT HUP INT TERM

  ar -t "$package_file" > "$listing_file"

  expected_members='debian-binary
control.tar.gz
data.tar.gz'

  actual_members=$(cat "$listing_file")

  [ "$actual_members" = "$expected_members" ] ||
    die "invalid Debian archive member order"

  (
    cd "$temporary_directory"
    ar -x "$package_file"
  )

  bsdtar -tf "$temporary_directory/control.tar.gz" > "$control_listing"
  bsdtar -tf "$temporary_directory/data.tar.gz" > "$data_listing"

  grep -Eq '^(\./)?control$' "$control_listing" ||
    die "missing Debian control file"

  grep -Eq '^(\./)?md5sums$' "$control_listing" ||
    die "missing Debian md5sums file"

  grep -Eq '^(\./)?postinst$' "$control_listing" ||
    die "missing Debian postinst script"

  grep -Eq '^(\./)?prerm$' "$control_listing" ||
    die "missing Debian prerm script"

  grep -Eq '^(\./)?postrm$' "$control_listing" ||
    die "missing Debian postrm script"

  grep -Eq '^(\./)?usr/local/bin/vitte$' "$data_listing" ||
    die "missing Vitte command in Debian package"

  grep -Eq '^(\./)?usr/local/share/vitte/editors/' "$data_listing" ||
    die "missing archived editor integrations in Debian package"

  grep -Eq '^(\./)?usr/local/share/vitte/completions/' "$data_listing" ||
    die "missing shell completions in Debian package"

  grep -Eq '^(\./)?usr/share/vim/vimfiles/syntax/vitte\.vim$' "$data_listing" ||
    die "missing Vim Vitte syntax in Debian package"

  grep -Eq '^(\./)?usr/share/vim/vimfiles/ftdetect/vitte\.vim$' "$data_listing" ||
    die "missing Vim Vitte filetype detection in Debian package"

  grep -Eq '^(\./)?usr/share/vim/vimfiles/ftplugin/vitte\.vim$' "$data_listing" ||
    die "missing Vim Vitte filetype plugin in Debian package"

  grep -Eq '^(\./)?usr/share/emacs/site-lisp/vitte/vitte-mode\.el$' "$data_listing" ||
    die "missing Emacs Vitte mode in Debian package"

  grep -Eq '^(\./)?etc/emacs/site-start\.d/50vitte\.el$' "$data_listing" ||
    die "missing Emacs Vitte automatic loader in Debian package"

  grep -Eq '^(\./)?usr/share/nano/vitte\.nanorc$' "$data_listing" ||
    die "missing Nano Vitte syntax in Debian package"

  grep -Eq '^(\./)?usr/share/geany/filedefs/filetypes\.Vitte\.conf$' "$data_listing" ||
    die "missing Geany Vitte filetype in Debian package"

  grep -Eq '^(\./)?usr/local/share/licenses/vitte/LICENSE$' "$data_listing" ||
    die "missing GNU GPLv3 license in Debian package"

  grep -Eq '^(\./)?usr/share/doc/vitte/copyright$' "$data_listing" ||
    die "missing Debian copyright file"

  grep -Eq '^(\./)?usr/local/share/vitte/assets/logo\.png$' "$data_listing" ||
    die "missing Vitte logo in Debian package"

  bsdtar -xOf \
    "$temporary_directory/data.tar.gz" \
    ./usr/local/share/licenses/vitte/LICENSE |
    grep -F 'GNU GENERAL PUBLIC LICENSE' >/dev/null ||
    die "invalid GNU GPLv3 license in Debian package"

  bsdtar -xOf \
    "$temporary_directory/data.tar.gz" \
    ./usr/local/share/licenses/vitte/LICENSE |
    grep -F 'Version 3, 29 June 2007' >/dev/null ||
    die "Debian package does not contain GNU GPL version 3"

  bsdtar -xOf \
    "$temporary_directory/data.tar.gz" \
    ./usr/share/vim/vimfiles/ftdetect/vitte.vim |
    grep -F '*.vit' >/dev/null ||
    die "Vim filetype detection does not register .vit"

  bsdtar -xOf \
    "$temporary_directory/data.tar.gz" \
    ./etc/emacs/site-start.d/50vitte.el |
    grep -F "\\.vit\\\\'" >/dev/null ||
    die "Emacs loader does not register .vit"

  bsdtar -xOf \
    "$temporary_directory/data.tar.gz" \
    ./usr/share/nano/vitte.nanorc |
    grep -E 'syntax[[:space:]]+"?[Vv]itte' >/dev/null ||
    die "Nano syntax file does not declare a Vitte syntax"

  rm -rf "$temporary_directory"
  rm -f "$listing_file" "$control_listing" "$data_listing"

  trap - EXIT HUP INT TERM
}

write_checksum() {
  package_file=$1

  scripts_build_sha256_write "$package_file" "$package_file.sha256"
}

build_one() {
  arch=$1

  stage=$ROOT_DIR/target/installer-linux-$arch
  control_root=$stage/control
  data_root=$stage/data
  package_file=$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${arch}.deb
  checksum_file=$package_file.sha256

  printf '[build-linux-debs] building Linux %s package\n' "$arch"

  rm -rf "$stage"
  rm -f "$package_file" "$checksum_file"

  mkdir -p \
    "$control_root" \
    "$data_root" \
    "$OUT_DIR"

  VERSION=$VERSION \
    SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH \
    "$PAYLOAD_SCRIPT" \
    "$data_root" \
    linux \
    "$arch" \
    unix

  install_editor_integrations "$data_root"
  add_license_and_logo "$data_root"
  verify_payload "$data_root" "$arch"
  normalize_payload "$data_root"

  installed_size=$(du -sk "$data_root" | awk '{print $1}')

  write_control_file \
    "$control_root" \
    "$arch" \
    "$installed_size"

  generate_md5sums \
    "$data_root" \
    "$control_root/md5sums"

  write_maintainer_scripts "$control_root"

  find "$control_root" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} + \
    2>/dev/null || true

  printf '2.0\n' > "$stage/debian-binary"

  create_tar_archive \
    "$stage/control.tar.gz" \
    "$control_root"

  create_tar_archive \
    "$stage/data.tar.gz" \
    "$data_root"

  touch -d "@$SOURCE_DATE_EPOCH" \
    "$stage/debian-binary" \
    "$stage/control.tar.gz" \
    "$stage/data.tar.gz" \
    2>/dev/null || true

  (
    cd "$stage"

    ar -qcS \
      "$package_file" \
      debian-binary \
      control.tar.gz \
      data.tar.gz
  )

  verify_debian_package "$package_file"
  write_checksum "$package_file"

  package_size=$(wc -c < "$package_file" | tr -d ' ')

  printf '[build-linux-debs] wrote %s (%s bytes)\n' \
    "$package_file" \
    "$package_size"

  printf '[build-linux-debs] wrote %s\n' \
    "$checksum_file"
}

require_file "$PACKAGE_VERSION_FILE" "PACKAGE_VERSION"

[ -x "$PAYLOAD_SCRIPT" ] ||
  die "payload staging script is missing or not executable: $PAYLOAD_SCRIPT"

require_directory "$EDITORS_DIR" "editor integrations"
require_directory "$COMPLETIONS_DIR" "shell completions"
verify_license_file "$LICENSE_FILE"
require_file "$LOGO_FILE" "Vitte logo"

for tool in \
  ar \
  awk \
  bsdtar \
  cat \
  cp \
  date \
  du \
  find \
  grep \
  install \
  mktemp \
  python3 \
  tar \
  touch \
  wc
do
  require "$tool"
done

case "$ARCH" in
  all)
    for arch in amd64 arm64 armhf armel i386 riscv64 ppc64el s390x mips64el mipsel powerpc sparc64; do
      build_one "$arch"
    done
    ;;
  x86_64 | X86_64 | amd64 | AMD64)
    build_one amd64
    ;;
  aarch64 | AArch64 | AARCH64 | arm64 | ARM64)
    build_one arm64
    ;;
  armhf | armv7 | armv7l)
    build_one armhf
    ;;
  armel | armv6)
    build_one armel
    ;;
  i386 | i486 | i586 | i686 | x86)
    build_one i386
    ;;
  riscv64 | RISC-V64 | RISCV64)
    build_one riscv64
    ;;
  ppc64el | powerpc64le | ppc64le)
    build_one ppc64el
    ;;
  s390x)
    build_one s390x
    ;;
  mips64el)
    build_one mips64el
    ;;
  mipsel)
    build_one mipsel
    ;;
  powerpc | ppc)
    build_one powerpc
    ;;
  sparc64)
    build_one sparc64
    ;;
  *)
    die "unsupported Linux architecture: $ARCH"
    ;;
esac

printf '[build-linux-debs] complete version=%s arch=%s out=%s\n' \
  "$VERSION" \
  "$ARCH" \
  "$OUT_DIR"
