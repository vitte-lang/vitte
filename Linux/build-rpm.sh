#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: Linux/build-rpm.sh [options]

Produces an RPM package for the Vitte CLI by compiling the binary (unless
--no-build is used) and driving rpmbuild with a generated spec file. The
resulting RPM is copied to Linux/dist/.

Options:
  --version X.Y.Z     Override the package version (default: from cargo metadata).
  --release N         Override the RPM release (default: 1).
  --target TRIPLE     Build artifacts for the given Rust target triple.
  --arch ARCH         Override the RPM architecture string (auto-derived from target).
  --depends LIST      Extra runtime dependencies (comma-separated).
  --no-build          Skip cargo build; reuse existing release artefacts.
  --keep-workdir      Keep the temporary staging directory.
  -h, --help          Show this message.

Environment:
  RPM_OUTPUT_DIR      Destination directory for generated RPMs (default: Linux/dist).

Dependencies:
  - cargo / rustc with appropriate targets installed
  - rpmbuild, gzip, jq, strip (optional)
EOF
}

log() { printf '▶ %s\n' "$*"; }
warn() { printf '⚠ %s\n' "$*" >&2; }
die() { warn "$*"; exit 1; }

require_cmd() {
  for cmd in "$@"; do
    command -v "$cmd" >/dev/null 2>&1 || die "Commande introuvable: $cmd"
  done
}

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

require_cmd cargo rustc jq rpmbuild gzip
STRIP_CMD="$(command -v strip || true)"

VERSION=""
RELEASE="1"
TARGET=""
ARCH_OVERRIDE=""
DEPENDS_EXTRA=""
BUILD=1
KEEP_WORKDIR=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)
      [[ $# -lt 2 ]] && die "--version requiert une valeur"
      VERSION="$2"
      shift 2
      ;;
    --release)
      [[ $# -lt 2 ]] && die "--release requiert une valeur"
      RELEASE="$2"
      shift 2
      ;;
    --target)
      [[ $# -lt 2 ]] && die "--target requiert une valeur"
      TARGET="$2"
      shift 2
      ;;
    --arch)
      [[ $# -lt 2 ]] && die "--arch requiert une valeur"
      ARCH_OVERRIDE="$2"
      shift 2
      ;;
    --depends)
      [[ $# -lt 2 ]] && die "--depends requiert une valeur"
      DEPENDS_EXTRA="$2"
      shift 2
      ;;
    --no-build)
      BUILD=0
      shift
      ;;
    --keep-workdir)
      KEEP_WORKDIR=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Option inconnue: $1"
      ;;
  esac
done

META_JSON="$(cargo metadata --format-version 1 --no-deps)"

if [[ -z "$VERSION" ]]; then
  VERSION="$(jq -r '.packages[] | select(.name=="vitte-bin") | .version' <<<"$META_JSON" | head -n1)"
  [[ -n "$VERSION" && "$VERSION" != "null" ]] || VERSION="0.1.0"
fi

LICENSE_TAG="$(jq -r '.packages[] | select(.name=="vitte-bin") | .license' <<<"$META_JSON" | head -n1)"
[[ -n "$LICENSE_TAG" && "$LICENSE_TAG" != "null" ]] || LICENSE_TAG="Apache-2.0"

HOMEPAGE="$(jq -r '.packages[] | select(.name=="vitte-bin") | .homepage' <<<"$META_JSON" | head -n1)"
[[ -n "$HOMEPAGE" && "$HOMEPAGE" != "null" ]] || HOMEPAGE="https://vitte-lang.github.io/vitte/"

PACKAGER="$(jq -r '.workspace_metadata.vitte.maintainer // empty' <<<"$META_JSON")"
if [[ -z "$PACKAGER" || "$PACKAGER" == "null" ]]; then
  NAME="$(git config user.name || true)"
  EMAIL="$(git config user.email || true)"
  if [[ -n "$NAME" && -n "$EMAIL" ]]; then
    PACKAGER="$NAME <$EMAIL>"
  else
    PACKAGER="Vitte Maintainers <maintainers@vitte-lang.org>"
  fi
fi

if [[ -z "$TARGET" ]]; then
  TARGET="$(rustc -Vv | awk '/host/ {print $2}')"
fi

rpm_arch_from_target() {
  case "$1" in
    x86_64-*) echo "x86_64" ;;
    aarch64-*) echo "aarch64" ;;
    armv7*-gnueabihf) echo "armv7hl" ;;
    arm-*-gnueabihf) echo "armv7hl" ;;
    i686-*) echo "i686" ;;
    riscv64-*) echo "riscv64" ;;
    *) warn "Architecture RPM inconnue pour '$1', utilisation telle quelle"; echo "$1" ;;
  esac
}

if [[ -z "$ARCH_OVERRIDE" ]]; then
  ARCH_OVERRIDE="$(rpm_arch_from_target "$TARGET")"
fi

RPM_OUTPUT_DIR="${RPM_OUTPUT_DIR:-Linux/dist}"
mkdir -p "$RPM_OUTPUT_DIR"

WORKDIR="$(mktemp -d -t vitte-rpm-XXXXXX)"
trap '[[ $KEEP_WORKDIR -eq 1 ]] || rm -rf "$WORKDIR"' EXIT

RPM_TOPDIR="$WORKDIR/rpmbuild"
mkdir -p "$RPM_TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

BIN_NAME="vitte"
BINARY_SOURCE_PATH=""

if (( BUILD )); then
  log "cargo build --release --target $TARGET"
  cargo build --release --target "$TARGET" --locked
fi

if [[ -f "$ROOT/target/$TARGET/release/vitte-bin" ]]; then
  BINARY_SOURCE_PATH="$ROOT/target/$TARGET/release/vitte-bin"
elif [[ -f "$ROOT/target/release/vitte-bin" ]]; then
  warn "Binaire ciblé non trouvé pour $TARGET, utilisation du build natif."
  BINARY_SOURCE_PATH="$ROOT/target/release/vitte-bin"
else
  die "Binaire vitte introuvable. Lancer cargo build ou spécifier --target."
fi

BIN_STAGED="$WORKDIR/assets/vitte-bin"
mkdir -p "$(dirname "$BIN_STAGED")"
cp "$BINARY_SOURCE_PATH" "$BIN_STAGED"
chmod 755 "$BIN_STAGED"

if [[ -n "$STRIP_CMD" ]]; then
  log "strip $BIN_STAGED"
  "$STRIP_CMD" "$BIN_STAGED" || warn "strip a échoué, continuation..."
fi

LICENSE_STAGE="$WORKDIR/assets/LICENSE"
if [[ -f LICENSE ]]; then
  install -Dm0644 LICENSE "$LICENSE_STAGE"
else
  echo "License information not bundled upstream." > "$LICENSE_STAGE"
fi

DOC_STAGE="$WORKDIR/assets/BUILD.md"
if [[ -f docs/BUILD.md ]]; then
  install -Dm0644 docs/BUILD.md "$DOC_STAGE"
else
  echo "Refer to https://vitte-lang.github.io/vitte/ for documentation." > "$DOC_STAGE"
fi

RPM_REQUIRES=("glibc >= 2.31")
if [[ -n "$DEPENDS_EXTRA" ]]; then
  IFS=',' read -ra EXTRA_DEPS <<< "$DEPENDS_EXTRA"
  for dep in "${EXTRA_DEPS[@]}"; do
    dep_trimmed="$(echo "$dep" | xargs)"
    [[ -z "$dep_trimmed" ]] && continue
    RPM_REQUIRES+=("$dep_trimmed")
  done
fi
REQUIRES_BLOCK="$(printf 'Requires: %s\n' "${RPM_REQUIRES[@]}")"

DESCRIPTION="The Vitte CLI orchestrates compiler, runtime, and tooling workflows for the Vitte programming language."
CHANGELOG_DATE="$(date +"%a %b %d %Y")"

SPEC_FILE="$RPM_TOPDIR/SPECS/vitte.spec"

{
  echo "Summary: Vitte language toolchain CLI"
  echo "Name: vitte"
  echo "Version: ${VERSION}"
  echo "Release: ${RELEASE}%{?dist}"
  echo "License: ${LICENSE_TAG}"
  echo "URL: ${HOMEPAGE}"
  echo "BuildArch: ${ARCH_OVERRIDE}"
  echo "Provides: vitte"
  printf '%b' "$REQUIRES_BLOCK"
  echo "Packager: ${PACKAGER}"
  echo
  echo "%description"
  echo "${DESCRIPTION}"
  echo
  echo "%prep"
  echo ":"
  echo
  echo "%build"
  echo ":"
  echo
  echo "%install"
  echo "install -D -m 0755 ${BIN_STAGED} %{buildroot}%{_bindir}/vitte"
  echo "install -D -m 0644 ${LICENSE_STAGE} %{buildroot}%{_docdir}/%{name}/LICENSE"
  echo "install -D -m 0644 ${DOC_STAGE} %{buildroot}%{_docdir}/%{name}/BUILD.md"
  echo
  echo "%files"
  echo "%license %{_docdir}/%{name}/LICENSE"
  echo "%doc %{_docdir}/%{name}/BUILD.md"
  echo "%{_bindir}/vitte"
  echo
  echo "%changelog"
  echo "* ${CHANGELOG_DATE} ${PACKAGER} ${VERSION}-${RELEASE}"
  echo "- Packaged by build-rpm.sh"
} > "$SPEC_FILE"

log "rpmbuild -bb $SPEC_FILE"
rpmbuild -bb --define "_topdir ${RPM_TOPDIR}" "$SPEC_FILE"

RPM_ARTIFACT="$(find "$RPM_TOPDIR/RPMS" -name 'vitte-*.rpm' -print -quit)"
[[ -n "$RPM_ARTIFACT" ]] || die "Aucun RPM généré."

DEST_RPM="$RPM_OUTPUT_DIR/$(basename "$RPM_ARTIFACT")"
cp "$RPM_ARTIFACT" "$DEST_RPM"

if (( KEEP_WORKDIR )); then
  log "Workdir conservé: $WORKDIR"
else
  rm -rf "$WORKDIR"
fi

log "✅ RPM généré: $DEST_RPM"
