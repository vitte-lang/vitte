# vitte-analyzer

**Statique et sémantique pour le langage Vitte.**  
Crate interne du compilateur `vitte` dédiée à l’analyse syntaxique, sémantique et symbolique des unités de compilation.  
Fait partie du projet [Vitte Language](https://github.com/vitte-lang/vitte).

---

## 📦 Présentation

`vitte-analyzer` assure la phase d’analyse du compilateur :

- Validation syntaxique post-AST (structure cohérente)
- Résolution des symboles (`use`, `mod`, `fn`, `type`, etc.)
- Gestion des portées et du typage
- Inférence et vérification de types
- Analyse des dépendances inter-modules
- Détection d’erreurs et avertissements détaillés
- Interface publique vers `vitte-lsp` pour l’IDE et l’autocomplétion

---

## 🧠 Architecture

| Module | Rôle principal |
|--------|----------------|
| `scope` | Gestion hiérarchique des portées (lexicales et globales) |
| `symbol` | Table des symboles et résolution des identifiants |
| `typeck` | Inférence et vérification de types |
| `diagnostics` | Emission d’erreurs, avertissements et notes |
| `visitor` | Parcours structuré des nœuds AST |
| `semantic` | Validation complète et transformation vers IR |
| `tests` | Cas unitaires et validation intégrée |

---

## ⚙️ Dépendances internes

```toml
[dependencies]
vitte-ast = { path = "../vitte-ast" }
vitte-core = { path = "../vitte-core" }
vitte-diagnostics = { path = "../vitte-diagnostics" }
vitte-utils = { path = "../vitte-utils" }
# vitte-analyzer

Moteur **d’analyse statique et sémantique** pour le langage Vitte.

`vitte-analyzer` constitue la couche d’intelligence du compilateur Vitte.  
Il effectue la validation syntaxique avancée, la résolution des symboles, la vérification des types et l’analyse des dépendances inter-modules.  
Ce crate est utilisé à la fois par le compilateur (`vitte-compiler`), le langage serveur (`vitte-lsp`) et les outils de documentation (`vitte-docgen`).

---

## Objectifs

- Analyse statique complète et rapide.
- Résolution symbolique (`use`, `mod`, `fn`, `type`, `const`, `trait`).
- Inférence et validation de types génériques.
- Analyse des dépendances entre unités de compilation.
- Génération de diagnostics précis (erreurs, avertissements, notes).
- API unifiée pour les IDE et outils externes.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `scope`        | Gestion hiérarchique des portées (lexicales et globales) |
| `symbol`       | Table des symboles et résolution des identifiants |
| `typeck`       | Vérification et inférence de types |
| `diagnostics`  | Emission d’erreurs, avertissements et conseils |
| `visitor`      | Parcours structuré de l’AST |
| `semantic`     | Analyse sémantique complète et validation IR |
| `graph`        | Dépendances inter-modulaires et cycles |
| `tests`        | Tests unitaires, snapshots et validation de flux |

---

## Exemple d’utilisation

```rust
use vitte_analyzer::Analyzer;
use vitte_ast::parse;

fn main() -> anyhow::Result<()> {
    let src = "fn add(a: Int, b: Int) -> Int { a + b }";
    let ast = parse(src)?;
    let mut analyzer = Analyzer::new();
    analyzer.analyze(&ast)?;
    println!("Analyse réussie !");
    Ok(())
}
```

---

## Intégration

- `vitte-lsp` : fournit les diagnostics et la complétion IDE.
- `vitte-compiler` : utilise les symboles et types validés.
- `vitte-docgen` : extrait les métadonnées de symboles pour la documentation.
- `vitte-analyzer-cli` : version autonome pour scripts et CI.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-diagnostics = { path = "../vitte-diagnostics", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

anyhow = "1"
serde = { version = "1.0", features = ["derive"] }
colored = "2"
indexmap = "2.5"
```

---

## Tests

```bash
cargo test -p vitte-analyzer
```

- Tests d’analyse sémantique.
- Vérification d’inférence de types.
- Détection de symboles non résolus.
- Snapshots de diagnostics complets.

---

## Roadmap

- [ ] Support des macros et dérivations personnalisées.
- [ ] Résolution avancée des traits et impl blocs.
- [ ] Analyse incrémentale pour IDE.
- [ ] Visualisation du graphe des dépendances.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau