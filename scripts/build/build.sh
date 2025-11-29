#!/bin/bash
# scripts/build/build.sh - Build complet du projet

set -euo pipefail

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN_DIR="$PROJECT_ROOT/bin"
OUT_DIR="$PROJECT_ROOT/out"
BUILD_DIR="$PROJECT_ROOT/build"
SRC_DIR="$PROJECT_ROOT/src"

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging
log() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Vérifications préalables
check_prerequisites() {
    log "Vérification des prérequis..."
    
    if [ ! -f "$BIN_DIR/vasm" ]; then
        error "vasm wrapper not found at $BIN_DIR/vasm"
        error "Run 'make bootstrap' first"
        exit 1
    fi
    
    if [ ! -d "$SRC_DIR" ]; then
        error "Source directory not found at $SRC_DIR"
        exit 1
    fi
    
    log "✓ Prérequis OK"
}

# Préparer les répertoires
prepare_directories() {
    log "Préparation des répertoires..."
    mkdir -p "$OUT_DIR" "$BUILD_DIR"
    log "✓ Répertoires préparés"
}

# Compiler les sources
compile_sources() {
    log "Compilation des sources..."
    
    # Compiler compilateur
    if [ -f "$SRC_DIR/tools/vittec.vitte" ]; then
        log "  Compilation du compilateur..."
        "$BIN_DIR/vasm" "$SRC_DIR/tools/vittec.vitte" \
            -o "$OUT_DIR/vittec.vbc" \
            || { error "Compilation vittec échouée"; exit 1; }
    fi
    
    # Compiler assembleur
    if [ -f "$SRC_DIR/tools/vasm.vitte" ]; then
        log "  Compilation de l'assembleur..."
        "$BIN_DIR/vasm" "$SRC_DIR/tools/vasm.vitte" \
            -o "$OUT_DIR/vasm.vbc" \
            || { error "Compilation vasm échouée"; exit 1; }
    fi
    
    # Compiler runtime
    if [ -f "$SRC_DIR/tools/vbc-run.vitte" ]; then
        log "  Compilation du runner bytecode..."
        "$BIN_DIR/vasm" "$SRC_DIR/tools/vbc-run.vitte" \
            -o "$OUT_DIR/vbc-run.vbc" \
            || { error "Compilation vbc-run échouée"; exit 1; }
    fi
    
    log "✓ Compilation réussie"
}

# Vérifier l'intégrité
verify_build() {
    log "Vérification de l'intégrité du build..."
    
    local build_ok=true
    for file in vittec.vbc vasm.vbc vbc-run.vbc; do
        if [ ! -f "$OUT_DIR/$file" ]; then
            warn "Missing $file"
            build_ok=false
        fi
    done
    
    if [ "$build_ok" = true ]; then
        log "✓ Build vérifié"
    else
        error "Build incomplet"
        exit 1
    fi
}

# Main
main() {
    log "Démarrage du build complet..."
    
    check_prerequisites
    prepare_directories
    compile_sources
    verify_build
    
    log "✓ Build complet réussi"
    echo ""
    log "Binaires compilés:"
    ls -lh "$OUT_DIR"/*.vbc 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
}

main "$@"
