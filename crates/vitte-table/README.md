

# vitte-table

Bibliothèque **de structures tabulaires et affichage de données formatées** pour l’écosystème Vitte.  
`vitte-table` fournit des outils pour créer, manipuler et afficher des tableaux dynamiques dans le terminal, les interfaces graphiques et les exports (Markdown, HTML, CSV).

---

## Objectifs

- Offrir une **API simple et performante** pour la manipulation tabulaire.  
- Supporter le rendu dans le terminal, le web et la documentation.  
- Intégration directe avec `vitte-cli`, `vitte-docgen`, `vitte-analyzer` et `vitte-profiler`.  
- Gérer le formatage adaptatif (couleurs, bordures, alignements, colonnes dynamiques).  
- Exporter les tableaux vers plusieurs formats standards.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `builder`     | Création dynamique de tableaux par API fluide |
| `renderer`    | Rendu terminal, HTML, Markdown et CSV |
| `style`       | Gestion des bordures, alignements, couleurs et polices |
| `cell`        | Modèle des cellules et contenu multi-type (texte, nombre, booléen) |
| `format`      | Helpers pour le formatage numérique, texte, unités |
| `export`      | Génération de fichiers `.md`, `.html`, `.csv` |
| `tests`       | Vérification du rendu et des performances |

---

## Exemple d’utilisation

```rust
use vitte_table::Table;

fn main() {
    let mut table = Table::new();
    table.set_header(["Nom", "Version", "Statut"]);
    table.add_row(["vitte-core", "0.1.0", "OK"]);
    table.add_row(["vitte-compiler", "0.1.0", "Buildé"]);

    println!("{}", table.render());
}
```

### Exemple de sortie terminal

```
+----------------+----------+----------+
| Nom            | Version  | Statut   |
+----------------+----------+----------+
| vitte-core     | 0.1.0    | OK       |
| vitte-compiler | 0.1.0    | Buildé   |
+----------------+----------+----------+
```

---

## Intégration

- `vitte-cli` : affichage tabulaire des résultats de commande.  
- `vitte-analyzer` : visualisation des dépendances et symboles.  
- `vitte-docgen` : génération des tableaux de documentation.  
- `vitte-profiler` : affichage des statistiques et mesures de performance.  
- `vitte-studio` : rendu dynamique dans les interfaces graphiques.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-style = { path = "../vitte-style", version = "0.1.0" }

comfy-table = "7.1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `tabled` pour rendu avancé, `csv` pour import/export, `html-escape` pour sécurité HTML.

---

## Tests

```bash
cargo test -p vitte-table
```

- Tests de rendu et alignement.  
- Tests d’export CSV/Markdown/HTML.  
- Tests de performance sur grands tableaux.  
- Validation de compatibilité terminal (TTY/non-TTY).

---

## Roadmap

- [ ] Fusion de cellules et multi-niveaux d’en-têtes.  
- [ ] Rendu graphique (SVG/Canvas) pour `vitte-studio`.  
- [ ] Export JSON pour API externes.  
- [ ] Support des couleurs adaptatives selon le thème.  
- [ ] Intégration directe avec `vitte-profiler` pour données temps réel.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau