# vitte-doc

Génération de documentation automatique pour **Vitte**.

Ce crate fournit les outils nécessaires pour extraire, transformer et générer la documentation des projets Vitte. Il permet de produire des pages HTML, Markdown, ou JSON à partir des commentaires de code, métadonnées et signatures des symboles analysés par le compilateur.

---

## Objectifs

- Offrir un système de documentation intégré au compilateur Vitte  
- Générer des sorties lisibles pour sites web, IDE et CLI  
- Supporter les formats HTML, Markdown et JSON  
- Extraire les signatures, types, paramètres et attributs du code source  
- Fournir un moteur extensible pour le style et la mise en page  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `extract`      | Extraction des symboles et commentaires depuis l’AST |
| `render`       | Rendu final (HTML, Markdown, JSON) |
| `template`     | Système de templates personnalisables |
| `index`        | Génération des index de symboles |
| `theme`        | Thèmes CSS/JS pour les pages HTML |
| `cli`          | Interface en ligne de commande |
| `tests`        | Tests de génération et rendu |

---

## Exemple d’utilisation

### Générer la documentation d’un projet

```bash
vitte doc build
```

Génère la documentation complète dans le répertoire `target/doc/`.

### Prévisualiser localement

```bash
vitte doc serve
```

Ouvre un serveur HTTP local pour explorer la documentation générée.

---

## Formats pris en charge

| Format   | Description |
|-----------|-------------|
| `html`    | Pages complètes avec navigation et styles |
| `markdown`| Documentation simple intégrable à un dépôt |
| `json`    | Sortie machine pour outils et LSP |

---

## Thèmes

Les thèmes sont situés dans `themes/` et peuvent être personnalisés :

```bash
vitte doc theme list
vitte doc theme use dark
vitte doc theme build
```

Thèmes intégrés :
- `light`
- `dark`
- `modern`
- `apple`
- `neoplastic` (inspiré de Jean Gorin)

---

## CLI

```
vitte doc build [--format <fmt>] [--theme <nom>] [--out <dossier>]
vitte doc serve [--port 8080]
vitte doc clean
vitte doc theme <commande>
```

Exemples :
```
vitte doc build --format html --theme dark
vitte doc serve --port 4000
vitte doc theme use apple
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-compiler = { path = "../vitte-compiler", version = "0.1.0" }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tera = "1.19"
pulldown-cmark = "0.10"
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-doc
vitte doc build --format markdown
```

- Tests d’intégration pour chaque format de sortie  
- Vérification des thèmes et templates  
- Tests de performances sur gros projets  

---

## Roadmap

- [ ] Génération de diagrammes automatiques  
- [ ] Support des extraits de code exécutables  
- [ ] Génération de doc API versionnée  
- [ ] Intégration CI avec `vitte-site`  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau

# vitte-docgen

Générateur de documentation **haut-niveau** pour l’écosystème Vitte.

`vitte-docgen` orchestre l’extraction des symboles, la résolution des liens, le rendu par thèmes et la production d’artefacts (HTML, Markdown, JSON, search-index). Il s’appuie sur `vitte-doc`, `vitte-ast` et `vitte-compiler` pour fournir une chaîne complète de documentation prête pour sites, wikis et IDE.

---

## Objectifs

- Pipeline complet *source → docs* avec un seul binaire.
- Thèmes, templates et shortcodes extensibles.
- Index de recherche incrémental et liens croisés entre crates.
- Sorties multi-formats: **html**, **md**, **json**.
- Mode *watch* avec rebuild partiel et serveur local.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `scan`         | Détection de projet, graph de dépendances, collecte d’entrées |
| `extract`      | Passage via `vitte-doc` pour extraire symboles et commentaires |
| `linker`       | Liens croisés, résolutions `see also`, anchors stables |
| `search`       | Construction de l’index (trigrammes + TF-IDF compact) |
| `render`       | Rendu final (HTML/MD/JSON) via templates |
| `theme`        | Gestion des thèmes, assets et variables |
| `watch`        | File watcher, rebuild partiel, cache |
| `server`       | Serveur HTTP local pour `docgen serve` |
| `cli`          | Interface en ligne de commande |
| `tests`        | Tests d’intégration et snapshots |

---

## Flux de génération

1. **Scan** du workspace et lecture de `Docgen.toml`.
2. **Extraction** des symboles via `vitte-doc`.
3. **Lien croisé** entre items et crates.
4. **Rendu** par thème choisi.
5. **Index** recherche + **assets** copiés.

Sortie par défaut dans `target/docgen/<format>/`.

---

## Fichier de configuration `Docgen.toml`

```toml
[project]
name = "vitte"
version = "0.1.0"

[build]
format = "html"        # html|markdown|json
theme  = "modern"      # light|dark|modern|apple|neoplastic
out_dir = "target/docsite"
incremental = true

[search]
enable = true
min_token = 2

[links]
external = { "rust" = "https://doc.rust-lang.org/std/" }

[markdown]
enable_emoji = false
code_line_numbers = true
```

---

## CLI

```
vitte docgen build [--format <fmt>] [--theme <nom>] [--out <dir>] [--no-search]
vitte docgen serve [--port 4000] [--open]
vitte docgen watch
vitte docgen clean
vitte docgen theme list|use <nom>|path
```

Exemples
```bash
vitte docgen build --format html --theme apple --out site/
vitte docgen serve --port 5173 --open
vitte docgen watch
```

---

## Thèmes et templates

- Thèmes fournis: `light`, `dark`, `modern`, `apple`, `neoplastic`.
- Moteur de templates: **Tera**.
- Shortcodes: `{% callout %}`, `{% apilink %}`, `{% since %}`.
- Variables disponibles: `project`, `item`, `breadcrumbs`, `sidebar`, `search_index_path`.

Personnalisation:
```bash
vitte docgen theme use modern
# Modifier themes/modern/templates/*.tera et themes/modern/assets/*
```

---

## Intégration

- `vitte-doc` pour l’extraction sémantique.
- `vitte-site` pour la publication statique.
- `vitte-lsp` pour l’index JSON consommable par IDE.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-doc  = { path = "../vitte-doc",  version = "0.1.0" }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tera = "1.19"
pulldown-cmark = "0.10"
notify = "6"
axum = "0.7"
tokio = { version = "1", features = ["full"] }
tower = "0.5"
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-docgen
vitte docgen build --format markdown
```

- Snapshots de rendu HTML/MD.
- Tests de liens cassés et d’ancres.
- Tests d’index de recherche.

---

## Roadmap

- [ ] Génération multi-langue et i18n.
- [ ] Diagrams (Mermaid) et API playground.
- [ ] Mode *PR preview* avec cache incrémental.
- [ ] Export Algolia-compatible pour la recherche.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau