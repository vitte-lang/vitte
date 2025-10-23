

# vitte-unicode

Bibliothèque **de gestion avancée du texte Unicode et des encodages** pour le langage Vitte.  
`vitte-unicode` fournit des outils pour la validation, la segmentation, la normalisation et la manipulation efficace du texte Unicode à toutes les étapes du compilateur et du runtime.

---

## Objectifs

- Offrir une **implémentation complète et performante du standard Unicode**.  
- Supporter la segmentation en graphèmes, mots et lignes.  
- Fournir la normalisation NFC, NFD, NFKC, NFKD selon le standard Unicode 15+.  
- Intégration directe avec `vitte-string`, `vitte-analyzer`, `vitte-compiler` et `vitte-lsp`.  
- Permettre la détection et conversion automatique des encodages (UTF-8, UTF-16, UTF-32).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `normalize`   | Fonctions de normalisation Unicode (NFC, NFD, etc.) |
| `segment`     | Découpage en graphèmes, mots, lignes |
| `encode`      | Conversion entre UTF-8/16/32 et vérification de validité |
| `detect`      | Détection automatique de l’encodage d’un flux texte |
| `tables`      | Tables Unicode statiques (catégories, blocs, scripts) |
| `tests`       | Validation de conformité et benchmarks |

---

## Exemple d’utilisation

```rust
use vitte_unicode::{normalize, segment};

fn main() -> anyhow::Result<()> {
    let text = "école";
    let normalized = normalize::to_nfc(text);
    let graphemes: Vec<_> = segment::graphemes(&normalized).collect();

    println!("{} → {:?}", normalized, graphemes);
    Ok(())
}
```

### Sortie

```
école → ["é", "c", "o", "l", "e"]
```

---

## Intégration

- `vitte-string` : opérations de texte et encodage.  
- `vitte-analyzer` : validation lexicale et normalisation des identifiants.  
- `vitte-compiler` : standardisation du texte source avant parsing.  
- `vitte-lsp` : segmentation Unicode pour le positionnement des diagnostics.  
- `vitte-docgen` : affichage correct des caractères combinés.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

unicode-normalization = "0.1"
unicode-segmentation = "1.11"
unicode-width = "0.1"
encoding_rs = "0.8"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `chardetng` pour détection d’encodage, `bstr` pour traitement binaire hybride.

---

## Tests

```bash
cargo test -p vitte-unicode
```

- Tests de conformité Unicode (graphemes, normalisation).  
- Tests de compatibilité avec `vitte-string` et `vitte-analyzer`.  
- Tests de performance sur grands fichiers texte.  
- Vérification multi-encodages (UTF-8, UTF-16, UTF-32).

---

## Roadmap

- [ ] Support du standard Unicode 16.  
- [ ] Détection automatique du script dominant.  
- [ ] Support des translittérations.  
- [ ] Visualisation des graphèmes combinés dans `vitte-studio`.  
- [ ] Intégration dans la pipeline du compilateur (`lexer` et `parser`).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau