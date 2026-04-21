# CLI Vitte

Utilisation :
```
vitte [command] [options] <input>
```

Commandes (recommandées) :
- `help`   Afficher l'aide et les tâches courantes
- `init [dir]`   Créer un squelette de projet minimal
- `explain <code>`  Expliquer un diagnostic (par ex. `E0001`)
- `doctor` Vérifier les prérequis de la chaîne d'outils
- `parse`  Parser uniquement (sans backend)
- `check`  Parser + résoudre + IR (sans backend)
- `emit`   Émettre uniquement le C++ (sans compilation native)
- `build`  Construction complète (par défaut)
- `profile`  Construction complète avec résumé des temps/mémoire par étape
- `reduce` Réduire un fichier en échec à un reproducer minimal
- `clean-cache`  Supprimer les fichiers de cache d'étape `.vitte-cache`
- `mod graph` Afficher le graphe d'import, les cycles et les poids de modules (LOC/exports/imports)
- `mod doctor` Vérifier l'hygiène des imports (non utilisés, alias, chemins canoniques, collisions)
- `mod contract-diff` Comparer le contrat exporté d'un module entre 2 entrées (`--old` / `--new`)

Options :
- `--lang <code>`  Langue des diagnostics (par ex. `en`, `fr`). Par défaut `LANG/LC_ALL`, puis `en`.
- `--explain <code>`  Expliquer un diagnostic (par ex. `E0001`).
- `--stage <name>`  S'arrêter à `parse|resolve|ir|backend`.
- `--diag-json`  Émettre les diagnostics en JSON.
- `--diag-json-pretty`  Émettre des diagnostics JSON lisibles (inclut `diag_schema`).
- `--diag-code-only`  Émettre des diagnostics compacts (`file:line:col CODE`), utile pour la CI.
- `--deterministic`  Ordre de sortie stable pour les diagnostics et outils.
- `--cache-report`  Afficher le rapport hit/miss du cache parse/resolve/ir.
- `--runtime-profile <name>`  Restreindre les imports runtime/stdlib : `core|system|desktop|arduino`.
- `--allow-experimental`  Autoriser les imports depuis les modules `experimental/...`.
- `--warn-experimental`  Dégrader un refus d'import expérimental en avertissement (mode migration).
- `--deny-internal`  Imposer la confidentialité des modules internes (comportement par défaut).
- `--allow-internal`  Désactiver la vérification de confidentialité des modules internes.
- `--strict-modules`  Interdire les imports glob et imposer une forme stricte des imports de module.
- `--dump-stdlib-map`  Afficher la carte résolue module stdlib -> symboles exportés (nom d'option historique).
- `--dump-module-index`  Exporter le JSON complet de l'index des modules (`modules`, `imports`, `exports`, `loc`).
- `--json`  Pour `mod graph` : émettre la charge JSON du graphe.
- `--from <module>`  Pour `mod graph` : limiter la vue au sous-graphe atteignable depuis le module.
- `--dump-ast`  Exporter l'AST après le parsing.
- `--dump-ir`  Exporter l'IR (`--dump-mir` en alias).
- `--strict-types`  Rejeter les alias de compatibilité (`integer`, `uint32`, etc.) et imposer les noms canoniques.
- `--strict-imports`  Exiger des alias d'import explicites, rejeter les alias inutilisés et interdire les chemins relatifs.
- `--fix`  Pour `mod doctor` : afficher des suggestions de réécriture concrètes.
- `--max-imports <N>`  Pour `mod doctor` : signaler les modules dont le fan-out dépasse `N`.
- `--old <file>` / `--new <file>`  Pour `mod contract-diff`.
- `--strict-bridge`  Alias de `--strict-imports` pour la politique de liaison native.
- `--fail-on-warning`  Traiter les avertissements comme des erreurs.
- `--stdout`  Émettre le C++ sur la sortie standard (implique `emit`).
- `--emit-obj`  Émettre un fichier objet natif (`.o`).
- `--repro`  Activer les drapeaux de sortie objet reproductible.
- `--repro-strict`  Imposer un ordre strictement déterministe pour le lowering IR.
- `--parse-modules`  Parser + charger les modules (sans résolution/lowering).
- `--parse-silent`  Supprimer les journaux d'information du mode parse-only.
- Les diagnostics incluent des codes d'erreur stables (par ex. `error[E0001]: ...`).

Examples:
```
vitte help
vitte init
vitte init app
vitte explain E0001
vitte doctor
vitte parse --lang=fr src/main.vit
```

Codes d'erreur :
- Les diagnostics utilisent des préfixes stables par couche :
  - `E000x` : parse/grammaire
  - `E100x` : résolution
  - `E1007` : cast signé/non signé invalide
  - `E1010..E1020` : contrat d'import module/stdlib
  - `E200x` : IR/lowering
  - `E300x` : backend/chaîne d'outils
- Les codes sont définis dans `src/compiler/frontend/diagnostics_messages.hpp`.
- Les fichiers de localisation `.ftl` peuvent traduire par code (préféré) ou par clé de message.

Stdlib :
- Voir `docs/book/stdlib.md` pour les modules stdlib et les exemples.
- Contrat et surface ABI : `docs/book/compiler-stdlib-contract.md`, `docs/book/stdlib_abi_surface_v1.txt`.

Options historiques (toujours prises en charge) :
- `mod api-diff` (alias de `mod contract-diff`)
- `--stdlib-profile` (alias historique de `--runtime-profile`)
  - correspondance : `minimal->core`, `kernel->system`, `full->desktop`
- `--parse-only`
- `--resolve-only`
- `--hir-only`
- `--mir-only`
- `--emit-cpp`
- `--strict-parse`

Notes :
- Les commandes définissent un mode par défaut, mais les options explicites restent prioritaires.
- Pour un parsing strict, utiliser `parse --strict-parse <file>`.
- Pour le JSON du graphe en CI : `vitte mod graph --json --from __root__ <file>`.
