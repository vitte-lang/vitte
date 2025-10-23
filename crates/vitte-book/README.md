

# vitte-book

Générateur **de documentation narrative et manuels officiels** pour l’écosystème Vitte.

`vitte-book` permet de compiler et publier la documentation technique du langage Vitte sous forme de livres, guides ou wikis interactifs.  
Il s’intègre à `vitte-docgen` pour produire un contenu structuré, versionné et prêt à être publié sur GitHub Pages, sites statiques ou IDE.

---

## Objectifs

- Générer des livres et guides complets à partir de sources Markdown ou Tera.  
- Intégration native avec `vitte-docgen` et les crates du projet.  
- Support des thèmes dynamiques (clair/sombre, moderne, Apple, Neoplastic).  
- Table des matières automatique et navigation contextuelle.  
- Liens croisés entre sections, symboles et API.  
- Support de la recherche plein texte et des références croisées.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `loader`       | Chargement des fichiers source (Markdown, Tera, JSON) |
| `parser`       | Transformation en AST documentaire |
| `renderer`     | Génération HTML/MD avec `vitte-docgen` |
| `toc`          | Construction dynamique de la table des matières |
| `theme`        | Gestion du thème et des assets (CSS, JS, fonts) |
| `search`       | Index de recherche incrémental |
| `watch`        | Rebuild partiel avec serveur local |
| `tests`        | Vérification du rendu et de la structure |

---

## Exemple d’utilisation

```bash
vitte book build --theme modern
vitte book serve --port 4000 --open
```

Depuis Rust :
```rust
use vitte_book::BookBuilder;

fn main() -> anyhow::Result<()> {
    let mut builder = BookBuilder::new("docs/");
    builder.theme("modern").output("target/book");
    builder.build()?;
    Ok(())
}
```

---

## Intégration

- `vitte-docgen` : pipeline de génération documentaire.  
- `vitte-site` : publication web statique.  
- `vitte-lsp` : consommation de l’index documentaire JSON.  
- `vitte-cli` : commandes `vitte book build/serve/watch`.  

---

## Fichier de configuration `Book.toml`

```toml
[book]
title = "Vitte Language Guide"
author = "Vincent Rousseau"
version = "0.1.0"
theme = "modern"
src = "docs"
out = "target/book"
search = true
```

---

## Dépendances

```toml
[dependencies]
vitte-core  = { path = "../vitte-core", version = "0.1.0" }
vitte-docgen = { path = "../vitte-docgen", version = "0.1.0" }
tera = "1.19"
pulldown-cmark = "0.10"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
notify = "6"
axum = "0.7"
tokio = { version = "1", features = ["full"] }
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-book
vitte book build --theme apple
```

- Tests de rendu HTML et navigation TOC.  
- Vérification des liens internes et ancres.  
- Snapshots de rendu multi-thèmes.  

---

## Roadmap

- [ ] Génération PDF et EPUB intégrée.  
- [ ] Mode interactif avec playground intégré.  
- [ ] Traductions multi-langue (i18n).  
- [ ] Export statique compatible GitHub Pages.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau