# Guide de Restructuration - Vitte Project

## 🎯 Objectifs

La restructuration vise à:
- ✅ Clarifier l'organisation logique
- ✅ Faciliter la navigation et la maintenance
- ✅ Standardiser les conventions
- ✅ Préparer la croissance futur
- ✅ Améliorer le build/test workflow

## 📊 État Actuel vs. État Cible

### État Actuel
```
vitte/ (root avec 47 fichiers et 20+ dossiers)
├── bootstrap/          ✅ Bien organisé
├── runtime/            ✅ Bien organisé (vient d'être complété)
├── compiler/           🚫 Peu utilisé
├── docs/               ✅ Présent
├── examples/           ✅ Présent
├── tests/              ✅ Présent
└── Beaucoup de fichiers MD au root
```

### État Cible
```
vitte/ (root épuré)
├── CORE/
│   ├── bootstrap/      ✅ (1989 LOC)
│   ├── runtime/        ✅ (2451 LOC)
│   └── compiler/       🚫 À compléter
├── REFERENCE/
│   ├── docs/
│   ├── spec/
│   └── grammar/
├── DEVELOPMENT/
│   ├── tests/
│   ├── examples/
│   └── benchmarks/
├── TOOLS/
│   ├── scripts/
│   ├── tools/
│   └── build system
└── Root docs (épuré)
```

## 🔄 Plan de Migration (Optionnel)

### Phase 1: Créer la Nouvelle Structure (Non-Destructive)
```bash
# 1. Créer les dossiers conteneurs
mkdir -p CORE REFERENCE DEVELOPMENT TOOLS

# 2. Créer les sous-dossiers
mkdir -p CORE/{bootstrap,runtime,compiler}
mkdir -p REFERENCE/{docs,spec,grammar}
mkdir -p DEVELOPMENT/{tests,examples,benchmarks}
mkdir -p TOOLS/{scripts,tools}
```

### Phase 2: Migrer les Fichiers (Option A: Symlinks)
```bash
# Pour les dossiers existants (conserve les originaux)
ln -s ../bootstrap CORE/bootstrap
ln -s ../runtime CORE/runtime
ln -s ../compiler CORE/compiler
ln -s ../docs REFERENCE/docs
ln -s ../spec REFERENCE/spec
ln -s ../grammar REFERENCE/grammar
ln -s ../examples DEVELOPMENT/examples
ln -s ../tests DEVELOPMENT/tests
ln -s ../scripts TOOLS/scripts
ln -s ../tools TOOLS/tools
```

### Phase 2: Migrer les Fichiers (Option B: Mouvements Physiques)
```bash
# ⚠️  À faire avec git pour tracer les changements
git mv bootstrap CORE/bootstrap
git mv runtime CORE/runtime
git mv compiler CORE/compiler
# ... etc
```

### Phase 3: Mettre à Jour les Références
- CMakeLists.txt
- build.sh
- Makefiles
- Scripts

### Phase 4: Épurer le Root
```bash
# Archiver les docs obsolètes
mkdir -p ARCHIVED_DOCS
mv FILE_INVENTORY.md ARCHIVED_DOCS/
mv ROOT_FILES_INDEX.md ARCHIVED_DOCS/
# ... etc
```

## 📋 Plan de Restructuration - Détails

### Répertoires Clés Actuels → Cibles

| Current | Target | Action |
|---------|--------|--------|
| `/bootstrap` | `/CORE/bootstrap` | Move ou symlink |
| `/runtime` | `/CORE/runtime` | Move ou symlink |
| `/compiler` | `/CORE/compiler` | Move ou symlink |
| `/docs` | `/REFERENCE/docs` | Move ou symlink |
| `/spec` | `/REFERENCE/spec` | Move ou symlink |
| `/grammar` | `/REFERENCE/grammar` | Move ou symlink |
| `/tests` | `/DEVELOPMENT/tests` | Move ou symlink |
| `/examples` | `/DEVELOPMENT/examples` | Move ou symlink |
| `/scripts` | `/TOOLS/scripts` | Move ou symlink |
| `/tools` | `/TOOLS/tools` | Move ou symlink |

### Fichiers Root à Organiser

**Root Documentation à Conserver:**
```
vitte/
├── README.md               → Vue d'ensemble
├── GETTING_STARTED.md      → Quick start
├── PROJECT_STRUCTURE.md    → Cette organisation
├── ARCHITECTURE.md         → Architecture globale
├── DESIGN.md              → Décisions de design
├── CONTRIBUTING.md         → Guide contribution
├── LICENSE                → Licence
├── VERSION                → Version
└── CHANGELOG.md           → Historique
```

**Root Documentation à Archiver:**
```
FILE_INVENTORY.md
ROOT_FILES_INDEX.md
README_START_HERE.md        (fusionner dans GETTING_STARTED.md)
IMPLEMENTATION_GUIDE.md     (déplacer dans REFERENCE/docs/)
IMPLEMENTATION_SUMMARY.md   (archiver)
SDK_*.md                   (archiver)
MANIFEST_SDK.md            (archiver)
```

### Build System Updates

**CMakeLists.txt Root**
```cmake
# Ajouter subdirectories pour la nouvelle structure
add_subdirectory(CORE/bootstrap)
add_subdirectory(CORE/runtime)
add_subdirectory(CORE/compiler)
```

**build.sh Updates**
```bash
# Mettre à jour les chemins
BOOTSTRAP_DIR="${PROJECT_ROOT}/CORE/bootstrap"
RUNTIME_DIR="${PROJECT_ROOT}/CORE/runtime"
COMPILER_DIR="${PROJECT_ROOT}/CORE/compiler"
```

## ⚙️ Implémentation Recommandée

### Option 1: Symlinks (Minimal, Réversible)
**Avantages:**
- Non-destructif
- Facile à revenir en arrière
- Pas de modification de git history
- Tests rapides sans migration réelle

**Commandes:**
```bash
cd /Users/vincent/Documents/Github/vitte
mkdir -p CORE REFERENCE DEVELOPMENT TOOLS/{scripts,tools}

ln -s ../bootstrap CORE/bootstrap
ln -s ../runtime CORE/runtime
ln -s ../compiler CORE/compiler
ln -s ../docs REFERENCE/docs
ln -s ../spec REFERENCE/spec
ln -s ../grammar REFERENCE/grammar
ln -s ../examples DEVELOPMENT/examples
ln -s ../tests DEVELOPMENT/tests
ln -s ../scripts TOOLS/scripts
ln -s ../tools TOOLS/tools
```

### Option 2: Git Moves (Permanent)
**Avantages:**
- Structure physique claire
- Historique git intact
- Configuration plus claire

**Procédure:**
```bash
git mv bootstrap CORE/bootstrap
git mv runtime CORE/runtime
git mv compiler CORE/compiler
git mv docs REFERENCE/docs
git mv spec REFERENCE/spec
git mv grammar REFERENCE/grammar
git mv examples DEVELOPMENT/examples
git mv tests DEVELOPMENT/tests
git mv scripts TOOLS/scripts
git mv tools TOOLS/tools

# Commit
git commit -m "chore: restructure project into CORE/REFERENCE/DEVELOPMENT/TOOLS"
```

## 🔗 Fichiers à Modifier

Après restructuration, mettre à jour:

1. **CMakeLists.txt**
   - Chemins subdirectories
   - include_directories
   - source file paths

2. **build.sh**
   - Variables de chemin
   - Appels de build

3. **scripts/***
   - Chemins bootstrap/runtime/compiler
   - Références croisées

4. **README.md**
   - Diriger vers PROJECT_STRUCTURE.md
   - Mettre à jour les chemins

5. **.gitignore**
   - Peut rester inchangé

## 📝 Checklist de Restructuration

- [ ] Créer la structure CORE/REFERENCE/DEVELOPMENT/TOOLS
- [ ] Appliquer symlinks OU git moves
- [ ] Mettre à jour CMakeLists.txt
- [ ] Mettre à jour build.sh
- [ ] Tester le build: `./build.sh --release`
- [ ] Tester les tests: `./build.sh --test`
- [ ] Vérifier les chemins bootstrap/runtime/compiler
- [ ] Archiver les docs obsolètes
- [ ] Mettre à jour README.md
- [ ] Commit et push (si git moves)
- [ ] Mise à jour de la documentation

## ✅ Validation Post-Restructuration

```bash
# Test compilation
cd vitte && ./build.sh --release

# Test bootstrap
./CORE/bootstrap/build.sh

# Test runtime
./CORE/runtime/build.sh

# Tous les tests
./build.sh --test

# Vérifier la structure
tree -L 2 -a
```

## 🚀 Bénéfices

1. **Navigation:** Structure logique et claire
2. **Maintenabilité:** Dépendances évidentes
3. **Scalabilité:** Facile d'ajouter de nouveaux composants
4. **Documentation:** Organisations des guides clairs
5. **Onboarding:** Nouveaux devs comprennent rapidement
6. **CI/CD:** Build/test/deploy simplifiés

## 📌 Notes Importantes

- La restructuration est **optionnelle** pour le fonctionnement
- Les symlinks permettent des tests sans modification permanente
- Les git moves conservent l'historique complet
- Les scripts existants continueront à marcher (avec mise à jour)
- Peut être fait progressivement, composant par composant

## 🆘 Rollback (si nécessaire)

```bash
# Si symlinks:
rm CORE REFERENCE DEVELOPMENT TOOLS
# Structure d'origine restée intacte

# Si git moves:
git revert <commit-hash>
git checkout -b restructured-before
git reset --hard <previous-commit>
```

---

**Recommandation:** Commencer avec les symlinks pour tester la structure sans risque, puis basculer vers des git moves si satisfait.
