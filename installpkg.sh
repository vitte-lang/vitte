#!/usr/bin/env bash
set -euo pipefail

# ==== Paramètres ====
GIT_URL="https://github.com/votre-org/vitte.git"
GIT_REF="main"
PKG_ID="com.vitte.lang"
VERSION="1.0.0"
FEATURES="${FEATURES:-}"
PREFIX="${PREFIX:-/usr/local}"
TARGETS="${TARGETS:-aarch64-apple-darwin}"
PROFILE="${PROFILE:-release}"

ROOT="$(pwd)"
BUILD="dist/pkg_net"
PAYLOAD="$BUILD/payload"
SCRIPTS="$BUILD/scripts"
PKG_OUT="$ROOT/vitte-installer-${VERSION}.pkg"

rm -rf "$BUILD"; mkdir -p "$PAYLOAD" "$SCRIPTS"

cat > "$SCRIPTS/preinstall" <<'PRE'
#!/usr/bin/env bash
set -euo pipefail
echo "[vitte] Pré-installation…"
exit 0
PRE
chmod +x "$SCRIPTS/preinstall"

cat > "$SCRIPTS/postinstall" <<'POST'
#!/usr/bin/env bash
set -euo pipefail

# Detect interactive TTY (installer usually runs headless)
INTERACTIVE=0
if [ -t 1 ]; then INTERACTIVE=1; fi

if [ "$INTERACTIVE" -eq 1 ]; then
  color(){ tput setaf "$1"; }
  reset(){ tput sgr0; }
else
  color(){ :; }
  reset(){ :; }
fi

# ── Fonctions utilitaires ──────────────────────────────────────────────
log(){ echo "$(color 6)[vitte]$(reset) $*"; }
fail(){ echo "$(color 1)[ERREUR]$(reset) $*" >&2; exit 1; }
need(){ command -v "$1" >/dev/null 2>&1 || fail "Dépendance manquante: $1"; }

# ── Progression simple ────────────────────────────────────────────────
progress() {
  local current=$1 total=$2 msg=$3
  local width=40
  local percent=$(( current * 100 / total ))
  local filled=$(( percent * width / 100 ))
  local empty=$(( width - filled ))
  if [ "$INTERACTIVE" -eq 1 ]; then
    printf "\r$(color 3)[%3d%%]$(reset) %s %s%s" "$percent" "$msg" "$(printf '#%.0s' $(seq 1 $filled))" "$(printf '.%.0s' $(seq 1 $empty))"
    if [ "$current" -eq "$total" ]; then echo; fi
  else
    echo "[${percent}%] $msg"
  fi
}

# ── Variables environnementales ───────────────────────────────────────
GIT_URL="${GIT_URL:-}"
GIT_REF="${GIT_REF:-main}"
PREFIX="${PREFIX:-/usr/local}"
FEATURES="${FEATURES:-}"
TARGETS="${TARGETS:-aarch64-apple-darwin}"
PROFILE="${PROFILE:-release}"

STEPS=7; step=0

# Étape 1 : Prérequis
step=$((step+1)); progress "$step" "$STEPS" "Vérification des dépendances..."
xcode-select -p >/dev/null 2>&1 || fail "Installe d'abord les Command Line Tools."
need git; need curl; sleep 0.3

# Étape 2 : Rustup
step=$((step+1)); progress "$step" "$STEPS" "Configuration de Rustup..."
if ! command -v rustup >/dev/null 2>&1; then
  log "Installation de rustup..."
  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y >/dev/null
  export PATH="$HOME/.cargo/bin:$PATH"
else
  export PATH="$HOME/.cargo/bin:$PATH"
fi
rustup toolchain install stable -c rustfmt -c clippy >/dev/null 2>&1 || true
rustup default stable >/dev/null 2>&1 || true; sleep 0.2

# Étape 3 : Cibles Rust
step=$((step+1)); progress "$step" "$STEPS" "Ajout des cibles Rust ($TARGETS)..."
for t in $TARGETS; do rustup target add "$t" >/dev/null 2>&1 || true; done; sleep 0.2

# Étape 4 : Clone du dépôt
step=$((step+1)); progress "$step" "$STEPS" "Téléchargement du dépôt..."
SRC_DIR="/usr/local/lib/vitte-src"
mkdir -p "$(dirname "$SRC_DIR")"
if [ -d "$SRC_DIR/.git" ]; then
  git -C "$SRC_DIR" fetch --all --tags >/dev/null 2>&1 || true
  git -C "$SRC_DIR" checkout "$GIT_REF" >/dev/null 2>&1 || true
  git -C "$SRC_DIR" pull >/dev/null 2>&1 || true
else
  git clone --depth 1 --branch "$GIT_REF" "$GIT_URL" "$SRC_DIR" >/dev/null 2>&1
fi; sleep 0.3

cd "$SRC_DIR"

# Étape 5 : Installation projet (install.sh / make install / cargo)
step=$((step+1)); progress "$step" "$STEPS" "Installation du projet..."
export PREFIX FEATURES TARGETS PROFILE
if [ -x "./scripts/install.sh" ]; then
  ./scripts/install.sh
elif [ -x "./install.sh" ]; then
  ./install.sh
elif grep -qE '(^|[^#])\binstall:' Makefile 2>/dev/null; then
  make install PREFIX="$PREFIX"
else
  for t in $TARGETS; do
    cargo build --release --target "$t" ${FEATURES:+--features "$FEATURES"} >/dev/null 2>&1
    OUT="target/$t/release"
    mkdir -p "$PREFIX/bin"
    find "$OUT" -maxdepth 1 -type f -perm -111 -print0 | xargs -0 -I{} install -m 0755 "{}" "$PREFIX/bin"
  done
fi; sleep 0.4

# Étape 6 : Vérification finale
step=$((step+1)); progress "$step" "$STEPS" "Vérification des binaires..."
ls "$PREFIX/bin" | grep vitte >/dev/null 2>&1 || fail "Aucun binaire Vitte trouvé."; sleep 0.3

# Étape 7 : Terminé
step=$((step+1)); progress "$step" "$STEPS" "Installation terminée ✔"
log "Installation complète dans $PREFIX/bin."
POST
chmod +x "$SCRIPTS/postinstall"

# ── Construction du PKG ───────────────────────────────────────────────
GIT_URL="$GIT_URL" GIT_REF="$GIT_REF" PREFIX="$PREFIX" FEATURES="$FEATURES" TARGETS="$TARGETS" PROFILE="$PROFILE" \
pkgbuild \
  --root "$PAYLOAD" \
  --identifier "$PKG_ID" \
  --version "$VERSION" \
  --install-location / \
  --scripts "$SCRIPTS" \
  "$PKG_OUT"

echo
echo "✅ $(tput bold)PKG créé :$(reset) $PKG_OUT"
echo "📡 Ce package clone le dépôt et affiche une progression en temps réel dans le terminal."