

# vitte-bin

Binaire principal du **langage Vitte**.  
`vitte-bin` regroupe les commandes utilisateur et les outils intégrés du compilateur : build, run, fmt, docgen, test, bench, et inspect.

---

## Objectifs

- Fournir une interface CLI complète et cohérente.  
- Centraliser l’accès à tous les sous-outils de l’écosystème (`vitte-*`).  
- Gérer la détection du workspace et le dispatch automatique des sous-commandes.  
- Intégrer les messages, bannières et couleurs du moteur `vitte-ansi`.  
- Support des plugins externes (`vitte plugin add <name>`).  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `main`         | Entrée du binaire, initialisation de la CLI |
| `args`         | Parsing des arguments (via `vitte-args`) |
| `dispatch`     | Appel des sous-commandes |
| `commands`     | Implémentations des outils (`build`, `fmt`, `docgen`, etc.) |
| `banner`       | Affichage d’accueil (via `vitte-banner`) |
| `env`          | Détection du workspace, configuration et cache |
| `plugin`       | Gestion des extensions CLI |
| `tests`        | Tests d’intégration et snapshots CLI |

---

## Exemple d’utilisation

```bash
vitte build
vitte run main.vit
vitte fmt --check
vitte docgen build --format html
vitte test
vitte inspect target/main.vitbc
```

---

## Sous-commandes disponibles

| Commande | Description |
|-----------|--------------|
| `build`   | Compile le projet courant |
| `run`     | Exécute un fichier source ou un binaire |
| `fmt`     | Formate le code source |
| `docgen`  | Génère la documentation |
| `bench`   | Exécute les benchmarks |
| `inspect` | Analyse un binaire `.vitbc` |
| `check`   | Vérifie la validité du projet |
| `repl`    | Lance un shell interactif |
| `clean`   | Supprime les artefacts de build |
| `plugin`  | Gère les extensions CLI |

---

## Intégration

- `vitte-args` : parsing et validation des options.  
- `vitte-banner` : affichage des informations d’accueil.  
- `vitte-cli` : moteur d’exécution des sous-commandes.  
- `vitte-core` : gestion des métadonnées de projet.  
- `vitte-tools` : sous-outils (`fmt`, `build`, `docgen`, `bench`).  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-cli = { path = "../vitte-cli", version = "0.1.0" }
vitte-args = { path = "../vitte-args", version = "0.1.0" }
vitte-banner = { path = "../vitte-banner", version = "0.1.0" }
vitte-tools = { path = "../vitte-tools", version = "0.1.0" }

anyhow = "1"
colored = "2"
serde = { version = "1.0", features = ["derive"] }
```

---

## Tests

```bash
cargo test -p vitte-bin
```

- Tests d’intégration CLI.  
- Vérification des sorties (`stdout`, `stderr`).  
- Snapshots des bannières et erreurs.  

---

## Roadmap

- [ ] Plugins tiers avec sandbox.  
- [ ] Support du mode serveur (`vitte daemon`).  
- [ ] Profilage et instrumentation intégrée.  
- [ ] Compatibilité Windows élargie.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau