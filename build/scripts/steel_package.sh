
#!/usr/bin/env sh
#
# steel_package.sh
#
# Release packager for the Steel/Vitte toolchain.
#
# Produces distributable archives for a given target profile/triple:
#   - toolchain binaries: steel, steelc
#   - headers: sdk/include/vitte/*.h (or include/steel/* if SDK not present)
#   - specs: spec/ (grammar, semantics, ABI docs)
#   - optional prebuilt stdlib (if present in target/)
#   - checksums: SHA-256
#
# Output layout:
#   target/releases/
#     vitte-<version>-<target>/
#       bin/
#       include/
#       spec/
#       std/              (optional)
#       muffin.muf        (workspace manifest snapshot)
#       VERSION
#       LICENSE
#       README.md         (minimal release readme)
#     vitte-<version>-<target>.tar.gz
#     vitte-<version>-<target>.zip
#     vitte-<version>-<target>.sha256
#
# Constraints:
#   - POSIX sh
#   - No network access
#   - Portable across BSD/Solaris/Linux/macOS
#
# Usage:
#   ./build/scripts/steel_package.sh --target darwin_arm64
#   ./build/scripts/steel_package.sh --target linux_x86_64 --format tar
#   ./build/scripts/steel_package.sh --format zip
#   ./build/scripts/steel_package.sh --include-std
#   ./build/scripts/steel_package.sh --sign none
#   ./build/scripts/steel_package.sh --out target/releases
#

set -eu

# ------------------------------- helpers ---------------------------------

say() { printf "%s\n" "$*"; }
warn() { printf "steel_package: %s\n" "$*" >&2; }
die() { printf "steel_package: %s\n" "$*" >&2; exit 2; }

abspath() {
  _p="$1"
  if [ -z "$_p" ]; then return 1; fi
  if [ -d "$_p" ]; then
    (cd "$_p" 2>/dev/null && pwd)
  else
    _d=$(dirname "$_p")
    _b=$(basename "$_p")
    (cd "$_d" 2>/dev/null && printf "%s/%s\n" "$(pwd)" "$_b")
  fi
}

is_cmd() { command -v "$1" >/dev/null 2>&1; }

mkdir_p() {
  _d="$1"
  [ -n "$_d" ] || return 0
  [ -d "$_d" ] || mkdir -p "$_d"
}

rm_rf() {
  _p="$1"
  [ -n "$_p" ] || return 0
  [ ! -e "$_p" ] || rm -rf "$_p"
}

cp_f() {
  _src="$1"; _dst="$2"
  mkdir_p "$(dirname "$_dst")"
  cp "$_src" "$_dst"
}

cp_r() {
  _src="$1"; _dst="$2"
  mkdir_p "$(dirname "$_dst")"
  cp -R "$_src" "$_dst"
}

# Portable file list copy: copy a list of files relative to ROOT -> dstroot
copy_rel_files() {
  _dstroot="$1"; shift
  while [ $# -gt 0 ]; do
    _rel="$1"; shift
    if [ -f "$ROOT_DIR/$_rel" ]; then
      cp_f "$ROOT_DIR/$_rel" "$_dstroot/$_rel"
    else
      warn "missing (skip): $_rel"
    fi
  done
}

# Prefer sha256sum, fallback shasum -a 256, fallback openssl dgst -sha256
sha256_file() {
  _f="$1"
  if is_cmd sha256sum; then
    sha256sum "$_f" | awk '{print $1}'
  elif is_cmd shasum; then
    shasum -a 256 "$_f" | awk '{print $1}'
  elif is_cmd sha256; then
    # BSD sha256
    sha256 -q "$_f"
  elif is_cmd openssl; then
    openssl dgst -sha256 "$_f" | awk '{print $2}'
  else
    return 1
  fi
}

# Escape spaces minimally for printing.
qpath() {
  printf "%s" "$1" | sed 's/ /\\ /g'
}

# Derive version
read_version() {
  if [ -f "$ROOT_DIR/VERSION" ]; then
    head -n 1 "$ROOT_DIR/VERSION" | tr -d '\r\n'
  elif [ -f "$ROOT_DIR/include/steel/version.h" ]; then
    # Best-effort: parse STEEL_VERSION_* defines
    _maj=$(grep -E 'STEEL_VERSION_MAJOR' "$ROOT_DIR/include/steel/version.h" 2>/dev/null | awk '{print $3}' | head -n1)
    _min=$(grep -E 'STEEL_VERSION_MINOR' "$ROOT_DIR/include/steel/version.h" 2>/dev/null | awk '{print $3}' | head -n1)
    _pat=$(grep -E 'STEEL_VERSION_PATCH' "$ROOT_DIR/include/steel/version.h" 2>/dev/null | awk '{print $3}' | head -n1)
    if [ -n "$_maj" ] && [ -n "$_min" ] && [ -n "$_pat" ]; then
      printf "%s.%s.%s" "$_maj" "$_min" "$_pat"
    else
      echo "0.0.0"
    fi
  else
    echo "0.0.0"
  fi
}

# Normalize target name
normalize_target() {
  _t="$1"
  if [ -n "$_t" ]; then
    echo "$_t"
    return 0
  fi

  # Default: infer from uname
  _os=$(uname -s 2>/dev/null || echo unknown)
  _arch=$(uname -m 2>/dev/null || echo unknown)
  case "$_os" in
    Darwin) _os=darwin ;;
    Linux) _os=linux ;;
    FreeBSD) _os=freebsd ;;
    NetBSD) _os=netbsd ;;
    OpenBSD) _os=openbsd ;;
    SunOS) _os=solaris ;;
  esac
  case "$_arch" in
    amd64|x86_64) _arch=x86_64 ;;
    arm64|aarch64) _arch=arm64 ;;
  esac

  echo "${_os}_${_arch}"
}

# Create a minimal README for the release bundle
write_release_readme() {
  _path="$1"
  _ver="$2"
  _tgt="$3"

  cat >"$_path" <<EOF
Steel/Vitte Toolchain Release

Version: ${_ver}
Target:  ${_tgt}

Contents:
  - bin/     : steel, steelc
  - include/ : public headers (SDK ABI + Steel internal headers)
  - spec/    : language + manifest specifications
  - std/     : optional prebuilt standard library

Quick start:
  - Ensure 'bin' is on PATH:
      export PATH="\$(pwd)/bin:\$PATH"

  - Build a Muffin workspace:
      steel build --config ./muffin.muf

Notes:
  - This bundle is produced by build/scripts/steel_package.sh
  - No network access is required to use the local toolchain.
EOF
}

usage() {
  cat <<EOF
Usage: steel_package.sh [options]

Options:
  --target <name>       target profile/triple name (default: inferred)
  --format <all|tar|zip>
                        archive format (default: all)
  --out <dir>           output directory (default: target/releases)
  --include-std         include std/ if present (prebuilt or sources)
  --include-sdk         include sdk/include/vitte if present (default: on)
  --no-sdk              do not include sdk headers
  --strip               attempt to strip binaries (best-effort)
  --clean               remove staging dir before packaging
  --sign <none>         signature mode placeholder (default: none)
  --verbose             more output
  --quiet               less output
  -h, --help            show this help
EOF
}

# ------------------------------- config ----------------------------------

ROOT_DIR=$(abspath "$(dirname "$0")/../..") || exit 1

TARGET_NAME=""
FORMAT="all"
OUT_DIR=""
INCLUDE_STD=0
INCLUDE_SDK=1
DO_STRIP=0
CLEAN=0
SIGN_MODE="none"
VERBOSE=0
QUIET=0

# ----------------------------- parse args --------------------------------

while [ $# -gt 0 ]; do
  case "$1" in
    --target) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; TARGET_NAME="$1"; shift ;;
    --format) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; FORMAT="$1"; shift ;;
    --out) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; OUT_DIR="$1"; shift ;;
    --include-std) INCLUDE_STD=1; shift ;;
    --include-sdk) INCLUDE_SDK=1; shift ;;
    --no-sdk) INCLUDE_SDK=0; shift ;;
    --strip) DO_STRIP=1; shift ;;
    --clean) CLEAN=1; shift ;;
    --sign) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; SIGN_MODE="$1"; shift ;;
    --verbose) VERBOSE=1; shift ;;
    --quiet) QUIET=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
  esac
done

# Validate
case "$FORMAT" in
  all|tar|zip) : ;;
  *) die "invalid --format: $FORMAT (expected all|tar|zip)" ;;
 esac

if [ "$SIGN_MODE" != "none" ]; then
  warn "--sign '$SIGN_MODE' not implemented (using none)"
  SIGN_MODE="none"
fi

TARGET_NAME=$(normalize_target "$TARGET_NAME")
VERSION=$(read_version)

if [ -z "$OUT_DIR" ]; then
  OUT_DIR="$ROOT_DIR/target/releases"
fi

OUT_DIR=$(abspath "$OUT_DIR") || exit 1
mkdir_p "$OUT_DIR"

if [ "$QUIET" -ne 1 ]; then
  say "steel_package: root=$ROOT_DIR"
  say "steel_package: version=$VERSION"
  say "steel_package: target=$TARGET_NAME"
  say "steel_package: out=$(qpath "$OUT_DIR")"
fi

# Ensure required workspace files exist
[ -f "$ROOT_DIR/muffin.muf" ] || die "missing muffin.muf at workspace root"

# Locate binaries
STEEL_BIN="$ROOT_DIR/target/bin/steel"
STEELC_BIN="$ROOT_DIR/target/bin/steelc"

if [ ! -x "$STEEL_BIN" ] || [ ! -x "$STEELC_BIN" ]; then
  die "missing toolchain binaries. Build first: ./build/scripts/steel_bootstrap.sh"
fi

# Optional strip
if [ "$DO_STRIP" -eq 1 ]; then
  if is_cmd strip; then
    if [ "$QUIET" -ne 1 ]; then
      say "steel_package: strip binaries (best-effort)"
    fi
    strip "$STEEL_BIN" >/dev/null 2>&1 || true
    strip "$STEELC_BIN" >/dev/null 2>&1 || true
  else
    warn "strip not found; skipping"
  fi
fi

# ------------------------------ staging ----------------------------------

PKG_NAME="vitte-${VERSION}-${TARGET_NAME}"
STAGE_DIR="$OUT_DIR/.stage/$PKG_NAME"

if [ "$CLEAN" -eq 1 ]; then
  rm_rf "$OUT_DIR/.stage"
fi

rm_rf "$STAGE_DIR"
mkdir_p "$STAGE_DIR"

# Layout
mkdir_p "$STAGE_DIR/bin"
mkdir_p "$STAGE_DIR/include"
mkdir_p "$STAGE_DIR/spec"

# Copy binaries
cp_f "$STEEL_BIN" "$STAGE_DIR/bin/steel"
cp_f "$STEELC_BIN" "$STAGE_DIR/bin/steelc"

# Copy core metadata
copy_rel_files "$STAGE_DIR" README.md LICENSE VERSION muffin.muf

# If workspace has a lockfile, include it as well (optional)
if [ -f "$ROOT_DIR/muffin.lock" ]; then
  cp_f "$ROOT_DIR/muffin.lock" "$STAGE_DIR/muffin.lock"
fi

# Write release README (overrides root README if included)
write_release_readme "$STAGE_DIR/README.md" "$VERSION" "$TARGET_NAME"

# Specs
if [ -d "$ROOT_DIR/spec" ]; then
  cp_r "$ROOT_DIR/spec" "$STAGE_DIR/spec"
else
  warn "missing spec/ directory"
fi

# Headers
# Prefer sdk/include/vitte for public ABI; also include include/steel/* for internal.
if [ "$INCLUDE_SDK" -eq 1 ] && [ -d "$ROOT_DIR/sdk/include" ]; then
  cp_r "$ROOT_DIR/sdk/include" "$STAGE_DIR/include/sdk"
fi

# Always include include/steel and include/vitte (if present)
if [ -d "$ROOT_DIR/include" ]; then
  cp_r "$ROOT_DIR/include" "$STAGE_DIR/include/src"
else
  warn "missing include/"
fi

# Optional std
if [ "$INCLUDE_STD" -eq 1 ]; then
  if [ -d "$ROOT_DIR/std" ]; then
    cp_r "$ROOT_DIR/std" "$STAGE_DIR/std"
  elif [ -d "$ROOT_DIR/target/std" ]; then
    cp_r "$ROOT_DIR/target/std" "$STAGE_DIR/std"
  else
    warn "std not found; skipping"
  fi
fi

# ------------------------------ archives ---------------------------------

ARCH_TAR="$OUT_DIR/$PKG_NAME.tar.gz"
ARCH_ZIP="$OUT_DIR/$PKG_NAME.zip"
SUM_FILE="$OUT_DIR/$PKG_NAME.sha256"

# Create tar.gz
make_tar() {
  if ! is_cmd tar; then
    die "tar not found"
  fi
  if [ "$QUIET" -ne 1 ]; then
    say "steel_package: tar -> $(qpath "$ARCH_TAR")"
  fi
  # Create in OUT_DIR; archive root contains PKG_NAME/
  (cd "$OUT_DIR/.stage" && tar -czf "$ARCH_TAR" "$PKG_NAME")
}

# Create zip
make_zip() {
  if ! is_cmd zip; then
    die "zip not found"
  fi
  if [ "$QUIET" -ne 1 ]; then
    say "steel_package: zip -> $(qpath "$ARCH_ZIP")"
  fi
  (cd "$OUT_DIR/.stage" && zip -r -q "$ARCH_ZIP" "$PKG_NAME")
}

# Checksums
write_sums() {
  if [ "$QUIET" -ne 1 ]; then
    say "steel_package: sha256 -> $(qpath "$SUM_FILE")"
  fi
  rm_rf "$SUM_FILE"
  : >"$SUM_FILE"

  if [ "$FORMAT" = "tar" ] || [ "$FORMAT" = "all" ]; then
    _h=$(sha256_file "$ARCH_TAR" 2>/dev/null || echo "")
    [ -n "$_h" ] || die "sha256 not available"
    printf "%s  %s\n" "$_h" "$(basename "$ARCH_TAR")" >>"$SUM_FILE"
  fi

  if [ "$FORMAT" = "zip" ] || [ "$FORMAT" = "all" ]; then
    _h=$(sha256_file "$ARCH_ZIP" 2>/dev/null || echo "")
    [ -n "$_h" ] || die "sha256 not available"
    printf "%s  %s\n" "$_h" "$(basename "$ARCH_ZIP")" >>"$SUM_FILE"
  fi
}

case "$FORMAT" in
  all)
    make_tar
    make_zip
    write_sums
    ;;
  tar)
    make_tar
    write_sums
    ;;
  zip)
    make_zip
    write_sums
    ;;
esac

# ------------------------------ final ------------------------------------

if [ "$QUIET" -ne 1 ]; then
  say "steel_package: done"
  case "$FORMAT" in
    all)
      say "  $(qpath "$ARCH_TAR")"
      say "  $(qpath "$ARCH_ZIP")"
      ;;
    tar) say "  $(qpath "$ARCH_TAR")" ;;
    zip) say "  $(qpath "$ARCH_ZIP")" ;;
  esac
  say "  $(qpath "$SUM_FILE")"
fi

exit 0
