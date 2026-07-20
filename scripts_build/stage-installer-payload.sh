#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
SCRIPT_NAME=stage-installer-payload
. "$ROOT_DIR/scripts_build/common.sh"
case "${1:-}" in
  --dry-run)
    shift
    DRY_RUN=1
    ;;
  --help | -h)
    printf '%s\n' "usage: stage-installer-payload.sh [--dry-run] DEST PLATFORM ARCH [unix|windows]"
    exit 0
    ;;
esac
DEST=${1:?usage: stage-installer-payload.sh DEST PLATFORM ARCH [unix|windows]}
PLATFORM=${2:?missing platform}
ARCH=${3:?missing architecture}
LAYOUT=${4:-unix}
VERSION=${VERSION:-$(tr -d ' \r\n' < "$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION")}
STRICT_PROCESSOR=${STRICT_PROCESSOR:-0}
scripts_build_maybe_dry_run "would stage installer payload platform=$PLATFORM arch=$ARCH layout=$LAYOUT dest=$DEST"

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
    -cf - -C "$source" . | tar -xf - -C "$destination"
  find "$destination" \( -name '.DS_Store' -o -name '._*' -o -name '.vitte-cache' -o -name '__pycache__' -o -name 'node_modules' \) -prune -exec rm -rf {} \; 2>/dev/null || true
}

normalize_key() {
  printf '%s' "$1" | tr '[:lower:]-' '[:upper:]_'
}

host_processor_label() {
  case "$(uname -m 2>/dev/null || printf unknown)" in
    x86_64 | amd64) printf 'amd64' ;;
    arm64 | aarch64) printf 'arm64' ;;
    i386 | i486 | i586 | i686) printf 'i386' ;;
    armv6*) printf 'armv6' ;;
    armv7*) printf 'armv7' ;;
    riscv64*) printf 'riscv64' ;;
    ppc64le | powerpc64le) printf 'powerpc64le' ;;
    ppc64 | powerpc64) printf 'powerpc64' ;;
    ppc | powerpc) printf 'powerpc' ;;
    sparc64*) printf 'sparc64' ;;
    *) printf 'unknown' ;;
  esac
}

find_command_payload() {
  command=$1
  platform_key=$(normalize_key "$PLATFORM")
  arch_key=$(normalize_key "$ARCH")
  command_key=$(normalize_key "$command")

  eval "override=\${VITTE_${command_key}_${platform_key}_${arch_key}:-}"
  [ -z "${override:-}" ] || {
    printf '%s\n' "$override"
    return 0
  }

  eval "override=\${VITTE_${command_key}_${arch_key}:-}"
  [ -z "${override:-}" ] || {
    printf '%s\n' "$override"
    return 0
  }

  eval "override=\${VITTE_BIN_${platform_key}_${arch_key}:-}"
  [ -z "${override:-}" ] || {
    printf '%s\n' "$override"
    return 0
  }

  eval "override=\${VITTE_BIN_${arch_key}:-}"
  [ -z "${override:-}" ] || {
    printf '%s\n' "$override"
    return 0
  }

  for candidate in \
    "$ROOT_DIR/target/$PLATFORM-$ARCH/$command" \
    "$ROOT_DIR/target/$PLATFORM-$ARCH/vitte" \
    "$ROOT_DIR/target/$ARCH/$command" \
    "$ROOT_DIR/bin/$PLATFORM-$ARCH/$command" \
    "$ROOT_DIR/bin/$ARCH/$command"
  do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  host_arch=$(host_processor_label)
  if [ "$ARCH" = "$host_arch" ] && [ -x "$ROOT_DIR/bin/$command" ]; then
    printf '%s\n' "$ROOT_DIR/bin/$command"
    return 0
  fi

  if [ -x "$ROOT_DIR/bin/$command" ] && [ "$STRICT_PROCESSOR" -eq 0 ]; then
    printf '%s\n' "$ROOT_DIR/bin/$command"
    return 0
  fi

  return 1
}

install_unix_command() {
  command=$1
  libexec_dir=$2
  bin_dir=$3
  payload=$(find_command_payload "$command") ||
    die "missing executable payload for $PLATFORM/$ARCH command $command; set VITTE_$(normalize_key "$command")_$(normalize_key "$PLATFORM")_$(normalize_key "$ARCH") or STRICT_PROCESSOR=0"

  install -m 0755 "$payload" "$libexec_dir/$command"
  cat > "$bin_dir/$command" <<EOF
#!/bin/sh
set -eu
export VITTE_ROOT=\${VITTE_ROOT:-/usr/local/share/vitte}
exec /usr/local/libexec/vitte/$command "\$@"
EOF
  chmod 0755 "$bin_dir/$command"
}

case "$LAYOUT" in
  unix)
    prefix=$DEST/usr/local
    bin_dir=$prefix/bin
    libexec_dir=$prefix/libexec/vitte
    share_dir=$prefix/share/vitte
    mkdir -p "$bin_dir" "$libexec_dir" "$share_dir"
    for command in vitte vittec vittec0; do
      install_unix_command "$command" "$libexec_dir" "$bin_dir"
    done
    copy_tree "$ROOT_DIR/man" "$prefix/share/man/man1"
    mkdir -p \
      "$prefix/etc/bash_completion.d" \
      "$prefix/share/zsh/site-functions" \
      "$prefix/share/fish/vendor_completions.d"
    [ ! -f "$ROOT_DIR/completions/bash/vitte" ] ||
      install -m 0644 "$ROOT_DIR/completions/bash/vitte" "$prefix/etc/bash_completion.d/vitte"
    [ ! -f "$ROOT_DIR/completions/zsh/_vitte" ] ||
      install -m 0644 "$ROOT_DIR/completions/zsh/_vitte" "$prefix/share/zsh/site-functions/_vitte"
    [ ! -f "$ROOT_DIR/completions/fish/vitte.fish" ] ||
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
    "components": [
        "compiler", "runtime", "stdlib", "sources", "documentation",
        "examples", "editors", "system-completions", "locales", "assets",
    ],
    "files": files,
}
manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

printf '[stage-installer-payload] complete platform=%s arch=%s layout=%s files=%s\n' \
  "$PLATFORM" "$ARCH" "$LAYOUT" "$(find "$DEST" -type f | wc -l | tr -d ' ')"
