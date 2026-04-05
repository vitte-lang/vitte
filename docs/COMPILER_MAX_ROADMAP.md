# Compiler MAX Roadmap

Objectif: exécuter l'ensemble du backlog compiler/parser en séquence contrôlée, avec garanties CI à chaque lot.

## Lot 1: Contrat Frontend (parser/diagnostics)
- [ ] Ajouter `--trace-parse` avec trace des règles et recoveries.
- [ ] Découper parser en modules (`toplevel/stmt/expr/type/pattern`).
- [ ] Ajouter `panic budget` parser (limite d'erreurs en cascade).
- [ ] Table de précédence déclarative unique (parser + docs + tests).
- [ ] Tests de précédence property-based.
- [ ] Étendre `grammar diff` vers `construct -> AST kind`.
- [ ] Ajouter `--dump-ast-json` + snapshots AST.
- [ ] Suite dédiée `error-recovery golden`.
- [ ] Rapport de couverture des sync tokens.
- [ ] Supprimer les diagnostics parser en texte libre (tout en `DiagId`).
- [ ] Suggestions `did you mean` sur patterns/types.

Definition of Done (Lot 1):
- `make grammar-check`, `make grammar-test`, `make core-language-gate` verts.
- Snapshots AST/diagnostics stabilisés et documentés.

## Lot 2: Diagnostics et UX
- [ ] Versionner explicitement le schéma JSON diagnostics.
- [ ] Valider ownership des codes diagnostics (tests + docs + locales).
- [ ] Lint bloquant: nouveau code diag sans explain/locales interdit.
- [ ] Taxonomie erreurs (`parse/resolve/type/lower/backend`) homogène.
- [ ] `--explain-last-error`.
- [ ] Suggestions fuzzy (Levenshtein) pour types/imports/codes.

Definition of Done (Lot 2):
- Contrat diagnostics publié + checks CI stricts.

## Lot 3: IR Golden et non-régression
- [ ] Suite `frontend contract` séparée du backend.
- [ ] IDs de phase internes (`P_PARSE_*`, etc.).
- [ ] Métriques AST/HIR dans `--profile`.
- [ ] Budgets perf CI par phase.
- [ ] Tests multi-fichiers imports/cycles/reexports renforcés.
- [ ] Golden MIR minimaux.
- [ ] Golden backend C++ normalisés.
- [ ] Générateur doc `Syntaxe -> AST -> HIR -> MIR`.

Definition of Done (Lot 3):
- Régression silencieuse parser/lowering détectée automatiquement.

## Lot 4: Gouvernance syntaxe
- [ ] Isoler strict-core en policy layer injectable.
- [ ] Manifeste versionné des profils syntaxiques (`syntax_profiles.toml`).
- [ ] Support profil syntaxe par fichier (pragma contrôlée).
- [ ] `--deny-experimental-syntax` séparé de strict-core.
- [ ] Graphe d'usage des features syntaxiques dans le repo.
- [ ] Harmoniser nomenclature AST/HIR/MIR (ex: match/select).
- [ ] Roadmap de stabilisation versionnée (`core-v1`, `stable-v1.1`, ...).

Definition of Done (Lot 4):
- Evolution syntaxique pilotée par versioning explicite et mesurable.

## Lot 5: Robustesse avancée (fuzz + crash)
- [ ] Fuzzing coverage-guided lexer/parser (libFuzzer/AFL harness).
- [ ] Corpus versionné de crash minimisés (`tests/fuzz_regressions`).
- [ ] Replay automatique des crash repro en CI.

Definition of Done (Lot 5):
- Campagne fuzz récurrente + zéro crash non trié dans la branche principale.

## Ordre de merge recommandé
1. Lot 1
2. Lot 2
3. Lot 3
4. Lot 4
5. Lot 5

## Règle de livraison
- Un PR = un sous-lot cohérent.
- Aucun merge sans tests/snapshots mis à jour.
- Aucun changement de diagnostic protégé sans mise à jour explicite des contrats.
