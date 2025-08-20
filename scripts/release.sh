#!/usr/bin/env bash
# scripts/release.sh — Orchestrateur de release pour le projet Vitte
# - Bump version (Cargo workspaces + extension VS Code + docs si présents)
# - Vérifs CHANGELOG, build, tests, packaging, checksums
# - Tag git + (optionnel) publication GitHub via `gh`
#
# Usage :
#   scripts/release.sh --version v0.2.0 [--dry-run] [--skip-tests] [--skip-ext] [--skip-docs] [--no-tag] [--no-push]
#   scripts/release.sh                      # déduit la prochaine version depuis le dernier tag (patch+1)
#
# Dépendances (best-effort) :
#   - git, sed, awk, grep, tar, sha256sum/shasum, jq (si ext), node+npm (si ext), vsce (si ext), cargo (si Rust)
#   - gh (optionnel pour publier la release)
#
# SPDX-License-Identifier: MIT

set -Eeuo pipefail

# ------------------------------- UX couleurs -------------------------------
is_tty() { [[ -t 1 ]]; }
have() { command -v "$1" >/dev/null 2>&1; }

if is_tty && have tput; then
  C_RESET="$(tput sgr0 || true)"
  C_BOLD="$(tput bold || true)"
  C_DIM="$(tput dim || true)"
  C_RED="$(tput setaf 1 || true)"
  C_GREEN="$(tput setaf 2 || true)"
  C_YELLOW="$(tput setaf 3 || true)"
  C_BLUE="$(tput setaf 4 || true)"
else
  C_RESET="" C_BOLD="" C_DIM="" C_RED="" C_GREEN="" C_YELLOW="" C_BLUE=""
fi

say()  { echo -e "${C_BOLD}${C_BLUE}▶${C_RESET} $*"; }
ok()   { echo -e "${C_GREEN}✓${C_RESET} $*"; }
warn() { echo -e "${C_YELLOW}⚠${C_RESET} $*"; }
die()  { echo -e "${C_RED}✗${C_RESET} $*" >&2; exit 1; }

# ------------------------------- Chemins -------------------------------
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
cd "$ROOT_DIR"

# ------------------------------- Options -------------------------------
VERSION=""           # vX.Y.Z (préfixe v supporté), sinon auto (patch+1)
DRY_RUN=0
SKIP_TESTS=0
SKIP_EXT=0
SKIP_DOCS=0
NO_TAG=0
NO_PUSH=0
CHANGELOG="CHANGELOG.md"
DOCS_BUILD_SCRIPT="scripts/gen_docs.sh"   # ajuste si besoin
EXT_DIR="editor-plugins/vscode"
EXT_PKG="${EXT_DIR}/package.json"
DIST_DIR="dist"

usage() {
  cat <<EOF
Usage: scripts/release.sh [options]

Options:
  --version vX.Y.Z   Version à publier (ex: v0.2.0). Si absent, auto (tag le + récent patch+1).
  --dry-run          N'exécute pas les opérations destructives (git tag/push, écriture fichiers).
  --skip-tests       Skip tous les tests (Cargo + vitc + npm).
  --skip-ext         Skip packaging extension VS Code.
  --skip-docs        Skip build docs.
  --no-tag           Ne crée pas le tag git.
  --no-push          Ne pousse pas vers l'origin (tag/branche).
  -h, --help         Affiche cette aide.
EOF
}

# Parse CLI
ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --version) VERSION="$2"; shift 2;;
    --dry-run) DRY_RUN=1; shift;;
    --skip-tests) SKIP_TESTS=1; shift;;
    --skip-ext) SKIP_EXT=1; shift;;
    --skip-docs) SKIP_DOCS=1; shift;;
    --no-tag) NO_TAG=1; shift;;
    --no-push) NO_PUSH=1; shift;;
    -h|--help) usage; exit 0;;
    *) ARGS+=("$1"); shift;;
  esac
done
set -- "${ARGS[@]}"

# ------------------------------- Helpers -------------------------------
semver_re='^v?([0-9]+)\.([0-9]+)\.([0-9]+)$'
norm_version() {
  local v="$1"
  [[ "$v" =~ ^v ]] && echo "$v" || echo "v${v}"
}

latest_tag() {
  git tag --list 'v*.*.*' --sort=-v:refname | head -n1
}

next_patch() {
  local lt rt maj min pat
  lt="$(latest_tag || true)"
  if [[ -z "$lt" ]]; then
    echo "v0.1.0"; return
  fi
  if [[ "$lt" =~ $semver_re ]]; then
    maj="${BASH_REMATCH[1]}"; min="${BASH_REMATCH[2]}"; pat="${BASH_REMATCH[3]}"
    printf "v%d.%d.%d\n" "$maj" "$min" "$((pat+1))"
  else
    echo "v0.1.0"
  fi
}

ensure_clean_git() {
  [[ -n "$(git status --porcelain)" ]] && die "Worktree sale. Commit/clean avant release."
  git fetch --tags --quiet || true
}

sed_inplace() {
  if [[ "$OSTYPE" == darwin* ]]; then
    sed -i '' "$@"
  else
    sed -i "$@"
  fi
}

sha256_file() {
  if have sha256sum; then sha256sum "$1"
  elif have shasum; then shasum -a 256 "$1"
  else
    die "Pas d'outil SHA256 (sha256sum/shasum)."
  fi
}

jq_set() {
  local file="$1" key="$2" val="$3"
  [[ ! -f "$file" ]] && die "Fichier introuvable: $file"
  tmp="${file}.tmp.$$"
  jq --arg v "$val" ".$key = \$v" "$file" > "$tmp"
  mv "$tmp" "$file"
}

# ------------------------------- Préflight -------------------------------
ensure_clean_git

if [[ -z "$VERSION" ]]; then
  VERSION="$(next_patch)"
fi
VERSION="$(norm_version "$VERSION")"
[[ "$VERSION" =~ $semver_re ]] || die "Version invalide '$VERSION' (attendu vX.Y.Z)."

say "Version ciblée : ${C_BOLD}${VERSION}${C_RESET}"

if git rev-parse "$VERSION" >/dev/null 2>&1; then
  die "Le tag ${VERSION} existe déjà."
fi

BRANCH="$(git rev-parse --abbrev-ref HEAD)"
say "Branche actuelle : ${C_DIM}${BRANCH}${C_RESET}"

# ------------------------------- Vérif CHANGELOG -------------------------------
if [[ -f "$CHANGELOG" ]]; then
  say "Mise à jour du CHANGELOG (${CHANGELOG})…"
  DATE="$(date +%Y-%m-%d)"
  # Remplace l'en-tête Unreleased par la version datée si nécessaire (souple).
  if grep -qE '## \[Unreleased\]' "$CHANGELOG"; then
    if [[ "$DRY_RUN" == "0" ]]; then
      sed_inplace "s/^## \\\[Unreleased\\\].*/## [Unreleased]\n\n## [${VERSION#v}] - ${DATE}/" "$CHANGELOG"
    else
      warn "(dry-run) maj CHANGELOG non écrite"
    fi
  else
    warn "Pas de section [Unreleased] détectée — je continue, mais vérifie ton changelog."
  fi
else
  warn "CHANGELOG.md absent — recommandé d'en tenir un."
fi

# ------------------------------- Bump versions (Cargo + extension) -------------------------------
say "Propagation de version dans le repo…"

if have cargo && [[ -f Cargo.toml ]]; then
  if have cargo-set-version; then
    say "cargo-set-version (workspace)…"
    [[ "$DRY_RUN" == "0" ]] && cargo set-version --workspace "${VERSION#v}" || warn "(dry-run) cargo set-version"
  else
    warn "cargo-set-version indisponible → tentative sed sur Cargo.toml"
    if [[ "$DRY_RUN" == "0" ]]; then
      # Bump root + crates (grossière mais efficace si format standard)
      grep -rl '^\s*version\s*=' --include='Cargo.toml' . | while read -r f; do
        sed_inplace "s/^\(\s*version\s*=\s*\"\)[0-9]\+\.[0-9]\+\.[0-9]\+\"/\1${VERSION#v}\"/" "$f"
      done
    else
      warn "(dry-run) sed Cargo.toml non appliqué"
    fi
  fi
else
  warn "Cargo.toml absent ou cargo non dispo — skip bump Rust."
fi

if [[ -d "$EXT_DIR" && -f "$EXT_PKG" ]]; then
  if have jq; then
    say "Mise à jour version extension VS Code (${EXT_PKG}) → ${VERSION#v}"
    [[ "$DRY_RUN" == "0" ]] && jq_set "$EXT_PKG" "version" "${VERSION#v}" || warn "(dry-run) jq_set"
  else
    warn "jq indisponible → tentative sed package.json"
    if [[ "$DRY_RUN" == "0" ]]; then
      sed_inplace "s/\"version\"\\s*:\\s*\"[0-9]\\+\\.[0-9]\\+\\.[0-9]\\+\"/\"version\": \"${VERSION#v}\"/" "$EXT_PKG"
    else
      warn "(dry-run) sed package.json non appliqué"
    fi
  fi
fi

# ------------------------------- Build & tests -------------------------------
if [[ "$SKIP_TESTS" == "1" ]]; then
  warn "Tests SKIPPED (--skip-tests)."
else
  if have cargo && [[ -f Cargo.toml ]]; then
    say "Cargo build --release…"
    [[ "$DRY_RUN" == "0" ]] && cargo build --workspace --release || warn "(dry-run) cargo build"
    say "Cargo test…"
    [[ "$DRY_RUN" == "0" ]] && cargo test --workspace || warn "(dry-run) cargo test"
  fi

  if have vitc; then
    say "vitc test…"
    [[ "$DRY_RUN" == "0" ]] && vitc test || warn "(dry-run) vitc test"
  fi

  if [[ -d "$EXT_DIR" && -f "$EXT_PKG" && "$SKIP_EXT" == "0" ]]; then
    if have npm; then
      say "Tests extension (npm)…"
      pushd "$EXT_DIR" >/dev/null
      if [[ -f package-lock.json ]]; then
        [[ "$DRY_RUN" == "0" ]] && npm ci || warn "(dry-run) npm ci"
      else
        [[ "$DRY_RUN" == "0" ]] && npm install || warn "(dry-run) npm install"
      fi
      if npm run | grep -q "test"; then
        [[ "$DRY_RUN" == "0" ]] && npm test || warn "(dry-run) npm test"
      else
        warn "Aucun npm test défini — skip."
      fi
      popd >/dev/null
    else
      warn "npm indisponible — skip tests extension."
    fi
  fi
fi

# ------------------------------- Docs -------------------------------
if [[ "$SKIP_DOCS" == "0" ]]; then
  if [[ -x "$DOCS_BUILD_SCRIPT" ]]; then
    say "Build docs via ${DOCS_BUILD_SCRIPT}…"
    [[ "$DRY_RUN" == "0" ]] && "$DOCS_BUILD_SCRIPT" || warn "(dry-run) docs build"
  elif [[ -d docs ]]; then
    say "Docs détectées (docs/) — aucun script de build. Skip."
  fi
else
  warn "Docs SKIPPED (--skip-docs)."
fi

# ------------------------------- Packaging -------------------------------
say "Packaging artefacts…"
REL_DIR="${DIST_DIR}/release-${VERSION}"
[[ "$DRY_RUN" == "0" ]] && mkdir -p "$REL_DIR" || true

# 1) Binaries Rust (si présents)
if [[ -d target/release ]]; then
  # archive brute du dossier release (pragmatique). Adapte si tu veux sélectionner des binaires précis.
  if [[ "$DRY_RUN" == "0" ]]; then
    tar -C target -czf "${REL_DIR}/target-release.tar.gz" release
  fi
  ok "Binaire(s) Cargo packagés."
fi

# 2) Extension VS Code
if [[ -d "$EXT_DIR" && -f "$EXT_PKG" && "$SKIP_EXT" == "0" ]]; then
  if have npx; then
    say "Packaging VSIX…"
    pushd "$EXT_DIR" >/dev/null
    if [[ "$DRY_RUN" == "0" ]]; then
      if [[ -f package-lock.json ]]; then npm ci; else npm install; fi
      npx vsce package --no-dependencies --out "${ROOT_DIR}/${REL_DIR}/vitte-vscode-${VERSION#v}.vsix"
    else
      warn "(dry-run) vsce package"
    fi
    popd >/dev/null
    ok "VSIX prêt."
  else
    warn "npx/vsce indisponible — VSIX non généré."
  fi
fi

# 3) Checksums
if [[ "$DRY_RUN" == "0" ]]; then
  (
    cd "$REL_DIR"
    for f in *; do
      [[ -f "$f" ]] || continue
      sha256_file "$f" >> SHA256SUMS
    done
  )
  ok "SHA256SUMS généré."
else
  warn "(dry-run) pas de checksum."
fi

# ------------------------------- Commit changelog + versions -------------------------------
say "Commit des changements de version…"
if [[ "$DRY_RUN" == "0" ]]; then
  git add -A
  git commit -m "release: ${VERSION}"
else
  warn "(dry-run) git commit skip"
fi

# ------------------------------- Tag & push -------------------------------
if [[ "$NO_TAG" == "0" ]]; then
  say "Création tag ${VERSION}…"
  [[ "$DRY_RUN" == "0" ]] && git tag -a "${VERSION}" -m "Release ${VERSION}" || warn "(dry-run) tag skip"
else
  warn "--no-tag : pas de tag."
fi

if [[ "$NO_PUSH" == "0" ]]; then
  say "Push branche + tags…"
  if [[ "$DRY_RUN" == "0" ]]; then
    git push
    git push --tags
  else
    warn "(dry-run) push skip"
  fi
else
  warn "--no-push : aucun push effectué."
fi

# ------------------------------- GitHub release (optionnel) -------------------------------
if have gh; then
  say "Création de la release GitHub (optionnel)…"
  NOTES_FILE="${REL_DIR}/RELEASE_NOTES.md"
  if [[ -f "$CHANGELOG" ]]; then
    # extrait heuristique des notes pour la version
    awk -v ver="${VERSION#v}" '
      $0 ~ "^## \\[" ver "\\]" {flag=1; next}
      $0 ~ "^## \\[" && flag==1 {flag=0}
      flag==1 {print}
    ' "$CHANGELOG" > "$NOTES_FILE" || true
  fi
  [[ -s "$NOTES_FILE" ]] || echo -e "Release ${VERSION}\n\nSee CHANGELOG.md for details." > "$NOTES_FILE"

  if [[ "$DRY_RUN" == "0" ]]; then
    gh release create "$VERSION" \
      --title "$VERSION" \
      --notes-file "$NOTES_FILE" \
      "${REL_DIR}"/* || warn "gh release create a échoué."
  else
    warn "(dry-run) gh release create skip"
  fi
else
  warn "gh CLI non trouvé — création de release GitHub non effectuée."
fi

ok "Release ${VERSION} terminée. Artefacts dans ${REL_DIR}"
echo -e "${C_DIM}Astuce: publie manuellement si besoin, et vérifie la signature/ checksums.${C_RESET}"
