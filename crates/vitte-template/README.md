


# vitte-template

Système **de gestion des modèles (templates)** pour l’écosystème Vitte.  
`vitte-template` permet la génération dynamique de fichiers, structures de projet, documentation ou code source à partir de modèles paramétrables.

---

## Objectifs

- Offrir une **API flexible pour la génération de contenu à partir de templates**.  
- Supporter les modèles texte, HTML, Markdown, JSON et code source.  
- Intégration directe avec `vitte-docgen`, `vitte-build`, `vitte-cli` et `vitte-studio`.  
- Fournir un moteur de rendu rapide, sûr et compatible avec les variables contextuelles.  
- Gérer la hiérarchie de templates et l’injection de données dynamiques.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `engine`      | Cœur du moteur de rendu, parsing et évaluation des modèles |
| `context`     | Gestion des variables et bindings dynamiques |
| `loader`      | Chargement des fichiers de templates (local, HTTP, intégré) |
| `syntax`      | Définitions du langage de template (expressions, boucles, conditions) |
| `render`      | Rendu final en texte, HTML ou JSON |
| `tests`       | Validation des modèles et performances |

---

## Exemple d’utilisation

```rust
use vitte_template::Template;
use std::collections::HashMap;

fn main() -> anyhow::Result<()> {
    let mut ctx = HashMap::new();
    ctx.insert("project", "Vitte");
    ctx.insert("version", "0.1.0");

    let tpl = Template::from_str("Projet {{ project }} — version {{ version }}")?;
    let result = tpl.render(&ctx)?;

    println!("{}", result);
    Ok(())
}
```

### Sortie

```
Projet Vitte — version 0.1.0
```

---

## Intégration

- `vitte-docgen` : génération de fichiers Markdown et HTML.  
- `vitte-build` : création automatique de squelettes de projet.  
- `vitte-cli` : commande `vitte new` et modèles de modules.  
- `vitte-studio` : génération d’interfaces à partir de templates UI.  
- `vitte-analyzer` : instrumentation pour l’injection de données contextuelles.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

tera = "1.20"
handlebars = "5.1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `askama` pour rendu Rust natif, `glob` pour recherche de modèles, `walkdir` pour arborescences complètes.

---

## Tests

```bash
cargo test -p vitte-template
```

- Tests de rendu Tera/Handlebars.  
- Tests de performance et cohérence.  
- Tests d’intégration avec `vitte-docgen`.  
- Benchmarks de parsing et substitution.

---

## Roadmap

- [ ] Système de cache et de compilation de templates.  
- [ ] Support du rendu incrémental.  
- [ ] Intégration complète avec le thème `vitte-style`.  
- [ ] Génération interactive via `vitte-studio`.  
- [ ] Gestion des templates distants et partagés.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau