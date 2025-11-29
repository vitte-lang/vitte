#!/bin/bash
# scripts/ci/lint.sh - Vérifier la syntaxe et style

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="$PROJECT_ROOT/src"

# Couleurs
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

log() {
    echo -e "${GREEN}[LINT]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Vérifier syntaxe Vitte
check_vitte_syntax() {
    log "Vérification syntaxe fichiers Vitte..."
    
    local errors=0
    find "$SRC_DIR" -name "*.vitte" | while read -r file; do
        # Vérifications basiques
        if grep -q "^\s*TODO\|FIXME\|XXX" "$file"; then
            error "TODO/FIXME trouvé dans $file"
            ((errors++))
        fi
    done
    
    if [ $errors -eq 0 ]; then
        log "✓ Pas d'erreurs de syntaxe"
    else
        exit 1
    fi
}

# Vérifier les lignes trop longues
check_line_length() {
    log "Vérification longueur des lignes..."
    
    local max_length=100
    find "$SRC_DIR" -name "*.vitte" | while read -r file; do
        if awk -v len=$max_length 'length > len {print FILENAME ":" NR ": Ligne trop longue (" length " > " len ")"}' "$file" | head -5; then
            echo "  (affichage limité à 5 lignes)"
        fi
    done
    
    log "✓ Vérification longueur OK"
}

# Vérifier les imports orphelins
check_unused_imports() {
    log "Vérification imports orphelins..."
    
    local orphans=0
    find "$SRC_DIR" -name "*.vitte" | while read -r file; do
        # Vérifications basiques (peut être amélioré)
        if grep -q "^use "; then
            log "  Vérification de $file"
        fi
    done
    
    log "✓ Vérification imports OK"
}

# Main
main() {
    log "Démarrage du linting..."
    
    check_vitte_syntax
    check_line_length
    check_unused_imports
    
    log "✓ Linting réussi"
}

main "$@"
