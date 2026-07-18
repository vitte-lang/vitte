#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
DEST=${1:?usage: stage-installer-payload.sh DEST PLATFORM ARCH [unix|windows]}
PLATFORM=${2:?missing platform}
ARCH=${3:?missing architecture}
LAYOUT=${4:-unix}
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}

die() {
  printf '[stage-installer-payload][error] %s\n' "$*" >&2
  exit 1
}

copy_tree() {
  source=$1
  destination=$2
  [ -e "$source" ] || return 0
  mkdir -p "$destination"
  COPYFILE_DISABLE=1 tar \
    --exclude='.DS_Store' --exclude='._*' --exclude='.vitte-cache' --exclude='__pycache__' \
    -cf - -C "$source" . | tar -xf - -C "$destination"
}

case "$LAYOUT" in
  unix)
    prefix=$DEST/usr/local
    bin_dir=$prefix/bin
    libexec_dir=$prefix/libexec/vitte
    share_dir=$prefix/share/vitte
    mkdir -p "$bin_dir" "$libexec_dir" "$share_dir"
    for command in vitte vittec vittec0; do
      [ -x "$ROOT_DIR/bin/$command" ] || die "missing executable payload: bin/$command"
      install -m 0755 "$ROOT_DIR/bin/$command" "$libexec_dir/$command"
      cat > "$bin_dir/$command" <<EOF
#!/bin/sh
set -eu
export VITTE_ROOT=\${VITTE_ROOT:-/usr/local/share/vitte}
exec /usr/local/libexec/vitte/$command "\$@"
EOF
      chmod 0755 "$bin_dir/$command"
    done
    copy_tree "$ROOT_DIR/man" "$prefix/share/man/man1"
    mkdir -p "$prefix/etc/bash_completion.d" "$prefix/share/zsh/site-functions" "$prefix/share/fish/vendor_completions.d"
    install -m 0644 "$ROOT_DIR/completions/bash/vitte" "$prefix/etc/bash_completion.d/vitte"
    install -m 0644 "$ROOT_DIR/completions/zsh/_vitte" "$prefix/share/zsh/site-functions/_vitte"
    install -m 0644 "$ROOT_DIR/completions/fish/vitte.fish" "$prefix/share/fish/vendor_completions.d/vitte.fish"
    ;;
  windows)
    share_dir=$DEST/share/vitte
    mkdir -p "$DEST/bin" "$share_dir"
    ;;
  *) die "unsupported layout: $LAYOUT" ;;
esac

copy_tree "$ROOT_DIR/src/vitte" "$share_dir/src/vitte"
copy_tree "$ROOT_DIR/toolchain/seed" "$share_dir/toolchain/seed"
for directory in assets completions docs editors examples locales; do
  copy_tree "$ROOT_DIR/$directory" "$share_dir/$directory"
done
for file in LICENSE README.md CHANGELOG.md; do
  [ ! -f "$ROOT_DIR/$file" ] || install -m 0644 "$ROOT_DIR/$file" "$share_dir/$file"
done
printf '%s\n' "$VERSION" > "$share_dir/VERSION"

python3 - "$DEST" "$share_dir/INSTALLATION.json" "$PLATFORM" "$ARCH" "$VERSION" "$LAYOUT" <<'PY'
import hashlib
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
manifest_path = Path(sys.argv[2])
files = []
for path in sorted(root.rglob("*")):
    if path.is_file() and path != manifest_path:
        files.append({
            "path": path.relative_to(root).as_posix(),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "size": path.stat().st_size,
        })
manifest = {
    "schema": "org.vitte.installer-payload.v1",
    "platform": sys.argv[3],
    "architecture": sys.argv[4],
    "version": sys.argv[5],
    "layout": sys.argv[6],
    "components": ["compiler", "runtime", "stdlib", "sources", "documentation", "examples", "editors", "system-completions", "locales", "assets"],
    "files": files,
}
manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

printf '[stage-installer-payload] complete platform=%s arch=%s layout=%s files=%s\n' \
  "$PLATFORM" "$ARCH" "$LAYOUT" "$(find "$DEST" -type f | wc -l | tr -d ' ')"
