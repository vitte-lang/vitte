

# vitte-shell

Interface **shell interactive et scripting** pour l’écosystème Vitte.  
`vitte-shell` fournit un environnement de ligne de commande enrichi permettant d’exécuter des commandes, scripts et pipelines Vitte, avec complétion, coloration, et extensions dynamiques.

---

## Objectifs

- Offrir une **interface shell unifiée et programmable** pour Vitte.  
- Supporter les scripts `.vitsh` avec variables, conditions et boucles.  
- Intégration directe avec `vitte-cli`, `vitte-repl`, `vitte-runtime` et `vitte-mod`.  
- Fournir des capacités de scripting avancé avec appels système et pipelines.  
- Support complet du thème clair/sombre et du rendu interactif.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `parser`      | Analyse des commandes et scripts shell |
| `executor`    | Exécution des commandes et scripts internes/externes |
| `env`         | Gestion des variables d’environnement et du contexte |
| `prompt`      | Rendu du prompt et interactions utilisateur |
| `history`     | Gestion de l’historique des commandes |
| `scripts`     | Chargement, parsing et exécution des fichiers `.vitsh` |
| `tests`       | Validation d’exécution et compatibilité multi-plateforme |

---

## Exemple d’utilisation

### Lancer le shell interactif

```bash
vitte shell
```

### Exemple de session

```text
vitte> let name = "Jean"
vitte> echo "Bonjour $name"
Bonjour Jean
vitte> build project.vitte | run
```

### Exemple de script `.vitsh`

```vitsh
#!/usr/bin/env vitte shell
let files = ls src/*.vitte
for f in $files {
    build $f
}
```

---

## Intégration

- `vitte-cli` : commande `vitte shell` et scripts embarqués.  
- `vitte-runtime` : exécution directe des commandes compilées.  
- `vitte-repl` : mode interactif partagé.  
- `vitte-mod` : chargement dynamique des modules shell.  
- `vitte-studio` : intégration graphique du terminal Vitte.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

rustyline = "12.0"
crossterm = "0.27"
regex = "1.10"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `glob` pour expansion des motifs, `which` pour détection des exécutables, `colored` pour affichage enrichi.

---

## Tests

```bash
cargo test -p vitte-shell
```

- Tests d’exécution de commandes internes et externes.  
- Tests de parsing et d’expansion des variables.  
- Tests d’intégration avec le runtime et le REPL.  
- Benchmarks sur les scripts de build et pipelines.

---

## Roadmap

- [ ] Support complet des scripts multi-fichiers.  
- [ ] Autocomplétion intelligente basée sur le contexte.  
- [ ] Historique partagé entre shell, REPL et CLI.  
- [ ] Exécution distante via `vitte-remote-cache`.  
- [ ] Visualisation des pipelines dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau