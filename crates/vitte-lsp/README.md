

# vitte-lsp

Serveur **Language Server Protocol (LSP)** pour le langage Vitte.  
`vitte-lsp` fournit les fonctionnalités d’analyse, de complétion, de diagnostic et de navigation symbolique pour les éditeurs et IDE compatibles LSP.

---

## Objectifs

- Offrir un **serveur de langage complet et réactif** pour Vitte.  
- Supporter les principales fonctionnalités LSP : diagnostics, complétion, hover, goto-definition, rename, signature help.  
- Intégration directe avec `vitte-analyzer`, `vitte-compiler` et `vitte-docgen`.  
- Maintenir la compatibilité avec VS Code, Neovim, Sublime Text, etc.  
- Assurer des performances élevées via cache incrémental et worker pool.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `server`      | Boucle principale LSP (JSON-RPC, message loop, dispatch) |
| `protocol`    | Structures et implémentations du protocole LSP |
| `handlers`    | Gestion des requêtes LSP (hover, completion, diagnostics…) |
| `workspace`   | Gestion des fichiers ouverts et des contextes projet |
| `diagnostics` | Interface avec `vitte-analyzer` pour rapports d’erreurs |
| `features`    | Modules spécifiques (rename, formatting, go-to-symbol) |
| `async`       | Traitement parallèle et communication inter-thread |
| `tests`       | Tests de conformité et intégration avec clients LSP |

---

## Exemple d’utilisation

### Lancement manuel

```bash
vitte lsp
```

### Exemple de configuration VS Code

```json
{
  "name": "Vitte Language Server",
  "command": "vitte",
  "args": ["lsp"],
  "filetypes": ["vitte"],
  "rootPatterns": ["vitte.toml", ".git/"],
  "settings": {}
}
```

---

## Intégration

- `vitte-analyzer` : extraction des symboles et diagnostics.  
- `vitte-compiler` : compilation incrémentale pour feedback en temps réel.  
- `vitte-docgen` : génération automatique de tooltips enrichis.  
- `vitte-cli` : intégration du mode `vitte lsp`.  
- `vitte-lint` : ajout des warnings et suggestions stylistiques.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }
vitte-compiler = { path = "../vitte-compiler", version = "0.1.0" }

lsp-server = "0.7"
lsp-types = "0.96"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
tokio = { version = "1", features = ["full"] }
``` 

> Optionnel : `tower` pour pipeline asynchrone, `crossbeam` pour parallélisation.

---

## Tests

```bash
cargo test -p vitte-lsp
```

- Tests de conformité LSP (hover, diagnostics, rename).  
- Tests d’intégration avec clients VS Code et Neovim.  
- Tests de performance du cache incrémental.  
- Tests d’analyse en temps réel sur gros projets.

---

## Roadmap

- [ ] Support complet du protocole LSP 3.18.  
- [ ] Système d’indexation persistante du workspace.  
- [ ] Surbrillance sémantique avancée.  
- [ ] API de plugins pour fonctionnalités IDE étendues.  
- [ ] Mode de communication TCP (remote server).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau