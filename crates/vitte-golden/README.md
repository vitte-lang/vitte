

# vitte-golden

Système **de tests golden (snapshot testing)** pour l’écosystème Vitte.

`vitte-golden` fournit une infrastructure de comparaison automatique entre la sortie actuelle d’un programme et un résultat de référence stocké (snapshot). Il est utilisé dans `vitte-fmt`, `vitte-docgen`, `vitte-analyzer` et d’autres crates pour garantir la stabilité du rendu et des formats.

---

## Objectifs

- Tests reproductibles basés sur des snapshots textuels ou binaires.
- Comparaison tolérante aux différences mineures (espaces, nouvelles lignes).
- Outils de mise à jour automatique des snapshots.
- Intégration transparente dans `cargo test`.
- Support multi-format : texte, JSON, HTML, binaire.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `snapshot`     | Gestion des fichiers de référence et sauvegarde |
| `compare`      | Diff et vérification des résultats |
| `update`       | Mise à jour automatique des golden files |
| `report`       | Génération de rapports détaillés sur les différences |
| `cli`          | Interface ligne de commande pour audit et mise à jour |
| `tests`        | Cas de test complets avec fixtures |

---

## Exemple d’utilisation

```rust
use vitte_golden::assert_golden;

#[test]
fn test_render_output() {
    let output = render_markdown("# Hello Vitte");
    assert_golden("snapshots/render_hello.md", &output);
}
```

---

## Mode CLI

```bash
vitte golden check
vitte golden update
vitte golden diff snapshots/
```

---

## Fonctionnement

1. Lors du premier test, `vitte-golden` crée un fichier snapshot.
2. Lors des tests suivants, il compare la sortie actuelle à ce fichier.
3. En cas de divergence, un diff est affiché.
4. Le snapshot peut être mis à jour via `vitte golden update`.

---

## Dépendances

```toml
[dependencies]
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
diff = "0.1"
colored = "2"
walkdir = "2.5"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-golden
```

- Vérification des différences textuelles.
- Tests sur formats JSON/HTML/binaires.
- Validation de la mise à jour automatique.

---

## Roadmap

- [ ] Support des snapshots compressés (gzip).
- [ ] Comparaison tolérante au whitespace configurable.
- [ ] Sortie en diff HTML coloré.
- [ ] Intégration directe avec `vitte-fmt` et `vitte-docgen`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau