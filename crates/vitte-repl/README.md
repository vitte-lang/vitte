


# vitte-repl

Moteur **REPL (Read–Eval–Print Loop)** pour le langage Vitte.  
`vitte-repl` fournit une interface interactive permettant d’évaluer du code Vitte dynamiquement, d’inspecter les valeurs, et de gérer des sessions persistantes pour le développement, l’expérimentation et l’enseignement.

---

## Objectifs

- Offrir un **environnement d’exécution interactif complet** pour Vitte.  
- Supporter la compilation et l’évaluation à la volée.  
- Maintenir un état de session avec persistance des variables et imports.  
- Intégration directe avec `vitte-runtime`, `vitte-analyzer`, et `vitte-compiler`.  
- Fournir des API internes pour l’intégration dans `vitte-studio` et `vitte-cli`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `engine`      | Boucle REPL (lecture, évaluation, affichage, boucle) |
| `context`     | Gestion de l’état de la session et environnement global |
| `eval`        | Compilation et exécution des expressions à la volée |
| `print`       | Formatage et affichage des résultats |
| `history`     | Persistance des entrées utilisateur |
| `syntax`      | Validation et complétion du code source |
| `tests`       | Vérification de cohérence et intégration complète |

---

## Exemple d’utilisation

### Lancement du REPL

```bash
vitte repl
```

### Exemple de session

```text
Vitte v0.1.0 — mode interactif
>>> let x = 3
>>> x * 4
12
>>> fn square(n: int) -> int { n * n }
>>> square(x)
9
```

---

## Intégration

- `vitte-runtime` : évaluation des instructions et fonctions.  
- `vitte-analyzer` : validation syntaxique et sémantique en direct.  
- `vitte-compiler` : compilation incrémentale et exécution.  
- `vitte-cli` : commande `vitte repl` avec options (`--theme`, `--no-color`, `--log`).  
- `vitte-studio` : module REPL graphique avec persistances multiples.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }
vitte-compiler = { path = "../vitte-compiler", version = "0.1.0" }

rustyline = "12.0"
console = "0.15"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `crossterm` pour compatibilité terminal, `colored` pour surlignage syntaxique.

---

## Tests

```bash
cargo test -p vitte-repl
```

- Tests d’évaluation d’expressions simples et composées.  
- Tests de persistance d’état entre entrées.  
- Tests de complétion et coloration syntaxique.  
- Benchmarks d’exécution dynamique et latence.

---

## Roadmap

- [ ] Support multi-ligne complet (déclarations, blocs).  
  - Étendre le parseur interactif pour regrouper les entrées via indentation/terminaison explicite et détecter les blocs incomplets.  
  - Ajouter un buffer de compilation incremental (`vitte-analyzer` + `vitte-compiler`) capable de réévaluer uniquement les sections modifiées.  
  - Couvrir les cas limites (structures imbriquées, fonctions, tests) par des tests de sessions reproduisibles.  
- [x] Historique partagé entre sessions.  
  - Backend persistant basé sur un fichier JSON (`~/.vitte/repl_history.json`) avec chiffrement XOR optionnel.  
  - Chargement automatique à l’ouverture et enregistrement après chaque cellule évaluée.  
  - Suite de tests garantissant la persistance et le format reproductible.  
- [x] Visualisation graphique des valeurs.  
  - Représentation `ValueViz` sérialisable (JSON) couvrant matrices, arbres, traces.  
  - Commandes `:viz-json` / `:viz-ascii` dans le REPL fournissant un rendu ASCII et export JSON.  
  - Tests de snapshot ASCII garantissant la stabilité des rendus principaux.  
- [ ] Mode connecté à `vitte-lsp`.  
  - [x] Rechercher la session LSP active et resynchroniser les symboles/état de projet (`:lsp-connect`, `:lsp-status`, `:lsp-sync`).  
  - [x] Exposer une API socket/JSON-RPC pour partager diagnostics, complétions (`:lsp-complete`), hover (`:lsp-hover`) et navigation (`:lsp-goto`) depuis le REPL.  
  - [ ] Tester l’expérience bout-à-bout avec `vitte-lsp` (complétion, goto definition) dans un environnement intégré.  
- [ ] Profilage des évaluations en direct.  
  - Ajouter des hooks de mesure (temps, allocations, compteurs) côté `vitte-runtime`.  
  - Afficher les métriques sous forme de timeline/tableau et permettre l’export (CSV/JSON).  
  - Couvrir les régressions via benchmarks automatisés et assertions de budgets.

---

## Priorités produit

1. **Support multi-ligne** — indispensable pour une adoption avancée du REPL et la parité avec l’éditeur.
2. **Historique partagé** — améliore la continuité de travail entre sessions CLI/Studio.
3. **Profilage en direct** — clé pour le retour de performance immédiat.
4. **Mode connecté LSP** — nécessite stabilisation des APIs côté `vitte-lsp`.
5. **Visualisation graphique** — livraison progressive via feature flag `viz`.

Pour le suivi, créer cinq épics alignés sur les axes ci-dessus, chacun déclinant les sous-tâches listées (moteurs multi-ligne, backend historique, instrumentation runtime, bridge LSP, viewer graphique). Ces épics serviront de base à la planification produit/ingénierie.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
