#!/usr/bin/env bash
# ============================================================
# Vitte — scripts/release.sh
# Build multi-cibles + archives + checksums + (opt) signature + (opt) GitHub Release
#
# Usage (exemples) :
#   scripts/release.sh v0.3.0
#   scripts/release.sh v0.3.0 --targets "x86_64-unknown-linux-gnu aarch64-apple-darwin" --features "compiler vm"
#   scripts/release.sh v0.3.0 --dry-run
#   scripts/release.sh v0.3.0 --gh --notes-from gitlog --pre
#
# Options :
#   --targets "<list>"         Liste de triples (espaces). Défaut : auto selon OS.
#   --features "<list>"        Features Cargo à activer (ex: "compiler vm serde")
#   --no-default-features      Désactive les default-features
#   --profile <name>           Profil Cargo (release par défaut)
#   --bin <name>               Binaire(s) à packager (répétable)
#   --examples                 Inclure les examples compilés
#   --strip                    strip des exécutables si possible
#   --zstd                     Archive .tar.zst au lieu de .tar.gz si zstd dispo
#   --zip                      Archive .zip (Windows friendly)
#   --dist-dir <path>          Dossier de sortie (défaut: dist/)
#   --notes-from <none|gitlog|file>  Source des release notes (défaut: gitlog si tag existe)
#   --notes-file <path>        Fichier à utiliser si --notes-from file
#   --pre                      Marque la release comme pre-release (GitHub)
#   --gh                       Publie sur GitHub (nécessite gh auth)
#   --sign                     Signe les checksums avec GPG (gpg --detach-sign)
#   --dry-run                  N’exécute pas les étapes destructives/push
#   --help                     Aide
#
# Sorties :
#   dist/<name>-<ver>-<target>.(tar.gz|tar.zst|zip)
#   dist/<name>-<ver>-SHA256SUMS (+ .asc si --sign)
#   dist/RELEASE_NOTES.md
#
# Convention :
#   * VERSION = premier argument (ex: v0.3.0). Si "auto", on prend `git describe --tags`.
#   * Nom de projet = basename du repo (ou override via PROJECT_NAME)
# ============================================================

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# ---------- couleurs ----------
if [ -t 1 ]; then
  C_RED="\033[31m"; C_GRN="\033[32m"; C_YLW="\033[33m"; C_BLU="\033[34m"; C_DIM="\033[2m"; C_RST="\033[0m"
else
  C_RED=""; C_GRN=""; C_YLW=""; C_BLU=""; C_DIM=""; C_RST=""
fi
log(){  echo -e "${C_BLU}[release]${C_RST} $*"; }
ok(){   echo -e "${C_GRN}[ok]${C_RST} $*"; }
warn(){ echo -e "${C_YLW}[!]${C_RST} $*"; }
err(){  echo -e "${C_RED}[err]${C_RST} $*"; }

trap 'err "échec à la ligne $LINENO"; exit 1' ERR

# ---------- helpers ----------
have(){ command -v "$1" >/dev/null 2>&1; }
arch(){ uname -m 2>/dev/null || echo unknown; }
os(){
  case "$(uname -s 2>/dev/null || echo unknown)" in
    Linux*) echo linux ;;
    Darwin*) echo macos ;;
    CYGWIN*|MINGW*|MSYS*) echo windows ;;
    *) echo unknown ;;
  esac
}

# ---------- args ----------
VERSION="${1:-}"
shift || true

TARGETS=""
FEATURES=""
NO_DEFAULT=false
PROFILE="release"
BINS=()
INCLUDE_EXAMPLES=false
STRIP=false
ARCHIVE_FMT="targz"     # targz | tarzst | zip
DIST_DIR="dist"
NOTES_FROM="auto"       # auto -> gitlog si tag
NOTES_FILE=""
PRE_RELEASE=false
GITHUB=false
SIGN=false
DRY_RUN=false

while (( "$#" )); do
  case "$1" in
    --targets) TARGETS="${2:?}"; shift 2;;
    --features) FEATURES="${2:?}"; shift 2;;
    --no-default-features) NO_DEFAULT=true; shift;;
    --profile) PROFILE="${2:?}"; shift 2;;
    --bin) BINS+=("${2:?}"); shift 2;;
    --examples) INCLUDE_EXAMPLES=true; shift;;
    --strip) STRIP=true; shift;;
    --zstd) ARCHIVE_FMT="tarzst"; shift;;
    --zip)  ARCHIVE_FMT="zip"; shift;;
    --dist-dir) DIST_DIR="${2:?}"; shift 2;;
    --notes-from) NOTES_FROM="${2:?}"; shift 2;;
    --notes-file) NOTES_FILE="${2:?}"; shift 2;;
    --pre) PRE_RELEASE=true; shift;;
    --gh)  GITHUB=true; shift;;
    --sign) SIGN=true; shift;;
    --dry-run) DRY_RUN=true; shift;;
    --help)
      sed -n '1,/^set -euo pipefail/p' "$0" | sed 's/^# \{0,1\}//'
      exit 0;;
    *) err "Option inconnue: $1"; exit 1;;
  esac
done

# ---------- checks ----------
have cargo || { err "cargo introuvable. Installe rustup: https://rustup.rs"; }
have git   || { err "git introuvable."; }

PROJECT_NAME="${PROJECT_NAME:-$(basename "$ROOT")}"

# Version
if [[ -z "$VERSION" || "$VERSION" == "auto" ]]; then
  VERSION="$(git describe --tags --always --dirty 2>/dev/null || echo 0.0.0)"
  warn "VERSION auto = $VERSION"
fi
[[ "$VERSION" =~ ^v?[0-9]+(\.[0-9]+){1,2}(-.+)?$ ]] || warn "VERSION ne ressemble pas à un semver/tag: $VERSION"

# Targets
if [[ -z "$TARGETS" ]]; then
  case "$(os)" in
    linux)   TARGETS="x86_64-unknown-linux-gnu aarch64-unknown-linux-gnu" ;;
    macos)   TARGETS="aarch64-apple-darwin x86_64-apple-darwin" ;;
    windows) TARGETS="x86_64-pc-windows-msvc aarch64-pc-windows-msvc" ;;
    *)       TARGETS="$(rustc -vV | awk -F: '/host:/ {print $2}' | xargs)" ;;
  esac
  warn "TARGETS auto = $TARGETS"
fi

mkdir -p "$DIST_DIR"

# ---------- release notes ----------
NOTES_PATH="$DIST_DIR/RELEASE_NOTES.md"
gen_notes_gitlog() {
  local base_tag prev_tag
  base_tag="$(git describe --tags --abbrev=0 2>/dev/null || echo "")"
  if [[ -n "$base_tag" ]]; then
    prev_tag="$(git tag --sort=-creatordate | sed -n '2p')"
  fi
  {
    echo "# $PROJECT_NAME $VERSION"
    echo
    if git rev-parse "$VERSION^{tag}" >/dev/null 2>&1; then
      echo "_Tag_: $VERSION"
    else
      echo "_Commit_: $(git rev-parse --short HEAD)"
    fi
    echo
    echo "## Changes"
    if [[ -n "$base_tag" ]]; then
      git log --pretty=format:'- %h %s (%an)' "${prev_tag:-$base_tag}..HEAD"
    else
      git log --pretty=format:'- %h %s (%an)'
    fi
    echo
  } > "$NOTES_PATH"
}

case "$NOTES_FROM" in
  auto)
    if git rev-parse "$VERSION^{tag}" >/dev/null 2>&1; then
      gen_notes_gitlog
    else
      gen_notes_gitlog
    fi
    ;;
  gitlog) gen_notes_gitlog ;;
  file)
    [[ -n "$NOTES_FILE" && -f "$NOTES_FILE" ]] || { err "--notes-file invalide"; }
    cp "$NOTES_FILE" "$NOTES_PATH"
    ;;
  none)
    echo "# $PROJECT_NAME $VERSION" > "$NOTES_PATH"
    ;;
  *) err "Valeur inconnue pour --notes-from";;
esac
ok "Notes → $NOTES_PATH"

# ---------- build function ----------
build_one_target() {
  local triple="$1"
  log "Build [$triple] profil=$PROFILE features=[${FEATURES:-none}]"
  # Prépare la toolchain si nécessaire
  rustup target add "$triple" >/dev/null 2>&1 || true

  local CARGO_BUILD=(cargo build --profile "$PROFILE" --target "$triple")
  if ((${#BINS[@]})); then
    for b in "${BINS[@]}"; do CARGO_BUILD+=(--bin "$b"); done
  else
    CARGO_BUILD+=(--workspace --bins)
  fi
  [[ -n "$FEATURES" ]] && CARGO_BUILD+=(--features "$FEATURES")
  $NO_DEFAULT && CARGO_BUILD+=(--no-default-features)

  # Linkers rapides sur Linux si dispo (non bloquant)
  if [[ "$triple" == *"-linux-"* ]]; then
    if have mold; then export RUSTFLAGS="${RUSTFLAGS:-} -Clink-arg=-fuse-ld=mold"; fi
    if have ld.lld; then export RUSTFLAGS="${RUSTFLAGS:-} -Clink-arg=-fuse-ld=lld"; fi
  fi

  "${CARGO_BUILD[@]}"

  # Copy out executables
  local from="target/$triple/$PROFILE"
  local stage="$DIST_DIR/stage-$triple"
  mkdir -p "$stage/bin"

  if ((${#BINS[@]})); then
    for b in "${BINS[@]}"; do
      if [[ "$triple" == *"-windows-"* ]]; then
        cp "$from/$b.exe" "$stage/bin/" 2>/dev/null || cp "$from/$b" "$stage/bin/" 2>/dev/null || true
      else
        cp "$from/$b" "$stage/bin/" 2>/dev/null || cp "$from/$b.exe" "$stage/bin/" 2>/dev/null || true
      fi
    done
  else
    # détecte les exécutables (bit + .exe)
    find "$from" -maxdepth 1 -type f -perm -111 -exec cp {} "$stage/bin/" \; 2>/dev/null || true
    find "$from" -maxdepth 1 -type f -name "*.exe" -exec cp {} "$stage/bin/" \; 2>/dev/null || true
  fi

  if $INCLUDE_EXAMPLES; then
    mkdir -p "$stage/examples"
    find "$from/examples" -maxdepth 1 -type f -perm -111 -exec cp {} "$stage/examples/" \; 2>/dev/null || true
    find "$from/examples" -maxdepth 1 -type f -name "*.exe" -exec cp {} "$stage/examples/" \; 2>/dev/null || true
  fi

  # strip (best effort)
  if $STRIP; then
    if have strip; then
      find "$stage" -type f -perm -111 -exec strip {} \; 2>/dev/null || true
      find "$stage" -type f -name "*.exe" -exec strip {} \; 2>/dev/null || true
    elif have llvm-strip; then
      find "$stage" -type f -perm -111 -exec llvm-strip {} \; 2>/dev/null || true
    fi
  fi

  # meta
  echo "$VERSION" > "$stage/VERSION"
  echo "$triple"  > "$stage/TARGET"
  [[ -f LICENSE ]] && cp LICENSE* "$stage/" 2>/dev/null || true
  [[ -f README.md ]] && cp README.md "$stage/" || true

  # archive
  local base="${PROJECT_NAME}-${VERSION}-${triple}"
  case "$ARCHIVE_FMT" in
    zip)
      (cd "$DIST_DIR" && rm -f "${base}.zip" && zip -r "${base}.zip" "stage-$triple" -q)
      ;;
    tarzst)
      if have zstd; then
        (cd "$DIST_DIR" && rm -f "${base}.tar.zst" && tar -cf - "stage-$triple" | zstd -19 -T0 -o "${base}.tar.zst")
      else
        warn "zstd non dispo, fallback .tar.gz"
        (cd "$DIST_DIR" && rm -f "${base}.tar.gz" && tar -czf "${base}.tar.gz" "stage-$triple")
      fi
      ;;
    targz|*)
      (cd "$DIST_DIR" && rm -f "${base}.tar.gz" && tar -czf "${base}.tar.gz" "stage-$triple")
      ;;
  esac

  # clean stage
  rm -rf "$stage"
  ok "Pack → $DIST_DIR/${base}.(tar.gz|tar.zst|zip)"
}

# ---------- build matrix ----------
IFS=' ' read -r -a TARGET_ARR <<< "$TARGETS"
for t in "${TARGET_ARR[@]}"; do
  build_one_target "$t"
done

# ---------- checksums ----------
log "Checksums SHA256…"
SUMFILE="$DIST_DIR/${PROJECT_NAME}-${VERSION}-SHA256SUMS"
rm -f "$SUMFILE"
if have shasum; then
  (cd "$DIST_DIR" && shasum -a 256 ${PROJECT_NAME}-${VERSION}-*.tar.* ${PROJECT_NAME}-${VERSION}-*.zip 2>/dev/null | tee -a "$(basename "$SUMFILE")")
elif have sha256sum; then
  (cd "$DIST_DIR" && sha256sum ${PROJECT_NAME}-${VERSION}-*.tar.* ${PROJECT_NAME}-${VERSION}-*.zip 2>/dev/null | tee -a "$(basename "$SUMFILE")")
else
  warn "Aucun outil de checksum SHA256. Skipping."
fi
ok "SHA256SUMS → $SUMFILE"

# ---------- sign (optional) ----------
if $SIGN; then
  if have gpg; then
    log "Signature GPG des checksums…"
    if $DRY_RUN; then
      warn "[dry-run] gpg --detach-sign $SUMFILE"
    else
      gpg --batch --yes --armor --detach-sign "$SUMFILE"
      ok "Signature → ${SUMFILE}.asc"
    fi
  else
    warn "gpg non trouvé — skip --sign"
  fi
fi

# ---------- GitHub Release (optional) ----------
if $GITHUB; then
  if have gh; then
    log "Publication GitHub Release…"
    ART_GLOB=()
    while IFS= read -r -d '' f; do ART_GLOB+=("$f"); done < <(find "$DIST_DIR" -maxdepth 1 -type f \( -name "${PROJECT_NAME}-${VERSION}-*.tar.*" -o -name "${PROJECT_NAME}-${VERSION}-*.zip" -o -name "$(basename "$SUMFILE")"* \) -print0)

    GH_ARGS=(release create "$VERSION" --title "${PROJECT_NAME} ${VERSION}" --notes-file "$NOTES_PATH")
    $PRE_RELEASE && GH_ARGS+=(--prerelease)

    if $DRY_RUN; then
      warn "[dry-run] gh ${GH_ARGS[*]} ${ART_GLOB[*]}"
    else
      gh "${GH_ARGS[@]}" "${ART_GLOB[@]}"
      ok "Release créée sur GitHub: $VERSION"
    fi
  else
    warn "`gh` non installé ou non authentifié — skip --gh"
  fi
fi

ok "Release terminée. ✨"
echo -e "${C_DIM}Artefacts dans ${DIST_DIR}${C_RST}"
