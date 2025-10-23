# vitte-diag

Système de diagnostics, erreurs et rapports unifié pour **Vitte**.

Ce crate gère la création, la mise en forme et la propagation des diagnostics à travers les différentes couches du compilateur, du runtime et des outils CLI. Il fournit une infrastructure centralisée pour les erreurs, avertissements et messages d’information, avec gestion des spans et codes d’erreur normalisés.

---

## Objectifs

- Offrir une API stable pour la gestion d’erreurs et de diagnostics  
- Supporter les niveaux `error`, `warning`, `note`, `info`, `help`  
- Associer les messages à des **spans**, **fichiers**, **lignes** et **colonnes**  
- Fournir un rendu riche compatible terminal (ANSI) et JSON  
- Intégrer les diagnostics dans `vitte-compiler`, `vitte-cli`, `vitte-analyzer`  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `level`        | Niveaux de gravité et filtrage |
| `code`         | Codes d’erreur et messages associés |
| `span`         | Position, fichier, ligne et colonne |
| `message`      | Construction et formatage des diagnostics |
| `reporter`     | Collecte et affichage des diagnostics |
| `format`       | Rendu coloré (ANSI), brut ou JSON |
| `context`      | Association d’un environnement d’exécution |
| `cli`          | Commandes de visualisation et tri |
| `tests`        | Tests de rendu et conformité CLI |

---

## Exemple d’utilisation

```rust
use vitte_diag::{Diagnostic, Level, Span};

fn main() {
    let span = Span::new("main.vit", 10, 5, 10, 15);
    let diag = Diagnostic::new(Level::Error)
        .with_code("E0001")
        .with_message("variable non définie : x")
        .with_span(span)
        .with_help("déclarer la variable avant utilisation");

    println!("{}", diag.render_ansi());
}
```

Résultat console :
```
error[E0001]: variable non définie : x
  ┌─ main.vit:10:5
  │
10 │     y = x + 1
  │         ^
  │         variable non définie
  │
  = help: déclarer la variable avant utilisation
```

---

## Niveaux de diagnostic

| Niveau   | Description |
|-----------|-------------|
| `Error`   | Erreurs bloquantes |
| `Warning` | Avertissements non bloquants |
| `Note`    | Informations contextuelles |
| `Help`    | Conseils d’amélioration |
| `Info`    | Messages généraux |

---

## Sorties disponibles

- **Terminal ANSI** : avec couleurs et indentation contextuelle  
- **JSON** : pour intégration CI/CD ou IDE  
- **Texte brut** : pour logs et automatisation  

---

## CLI

```
vitte diag show
vitte diag list
vitte diag export --format json
vitte diag filter --level warning
```

Exemples :
```
vitte diag show --file main.vit
vitte diag export --format json > report.json
```

---

## Intégration

- `vitte-compiler` : génération des erreurs et warnings  
- `vitte-analyzer` : analyse statique avec diagnostics enrichis  
- `vitte-cli` : affichage utilisateur  
- `vitte-lsp` : renvoi structuré vers les IDE  
- `vitte-test` : comparaison de diagnostics avec snapshots  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
colored = "2"
unicode-width = "0.1"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-diag
vitte diag export --format json
```

- Tests d’affichage multi-plateforme  
- Tests de formatage et indentation ANSI  
- Tests de sérialisation JSON  

---

## Roadmap

- [ ] Support complet LSP (Language Server Protocol)  
- [ ] Stack de diagnostics chaînés (`caused by:`)  
- [ ] Rendu HTML pour CI/CD et docs automatiques  
- [ ] Diagnostic visuel dans `vitte-analyzer`  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
