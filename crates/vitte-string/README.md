# vitte-string

Bibliothèque **de manipulation avancée des chaînes de caractères** pour le langage Vitte.  
`vitte-string` fournit des types, fonctions et algorithmes optimisés pour la gestion des textes, encodages et opérations Unicode, tout en assurant performance et sécurité mémoire.

---

## Objectifs

- Offrir une **API performante et sûre** pour la gestion des chaînes Vitte.  
- Supporter les encodages UTF-8, UTF-16, et UTF-32 de manière transparente.  
- Fournir des fonctions de recherche, remplacement, découpe, normalisation et comparaison.  
- Intégration directe avec `vitte-std`, `vitte-utils`, `vitte-compiler` et `vitte-analyzer`.  
- Maintenir la compatibilité totale avec les conventions Unicode (NFC, NFD, NFKC, NFKD).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `types`       | Définition des structures `VString`, `StrView`, `CowStr` |
| `unicode`     | Outils Unicode : normalisation, segmentation, graphemes |
| `search`      | Fonctions de recherche (`find`, `rfind`, `contains`, KMP) |
| `split`       | Découpage et itération sur les sous-chaînes |
| `case`        | Conversion de casse, capitalisation et folding |
| `encode`      | Conversion entre encodages et validation UTF |
| `tests`       | Validation des performances et conformité Unicode |

---

## Exemple d’utilisation

```rust
use vitte_string::VString;

fn main() {
    let mut s = VString::from("Vitte est rapide et sûr!");
    println!("Longueur : {}", s.len());

    s.push_str(" Langage moderne.");
    let words: Vec<_> = s.split_whitespace().collect();
    println!("{} mots.", words.len());
}
```

---

## Intégration

- `vitte-std` : implémente les primitives textuelles de la bibliothèque standard.  
- `vitte-analyzer` : analyse sémantique et gestion des identifiants textuels.  
- `vitte-compiler` : traitement des littéraux et encodages de source.  
- `vitte-utils` : fonctions d’aide pour conversions et affichage.  
- `vitte-docgen` : génération de documentation textuelle et liens internes.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

unicode-segmentation = "1.11"
unicode-normalization = "0.1"
encoding_rs = "0.8"
regex = "1.10"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `aho-corasick` pour recherche rapide, `bstr` pour compatibilité binaire.

---

## Tests

```bash
cargo test -p vitte-string
```

- Tests d’encodage/décodage UTF.  
- Tests de normalisation et comparaison Unicode.  
- Tests de performance sur grandes chaînes.  
- Vérification de compatibilité inter-langage (C, Rust, Vitte).

---

## Roadmap

- [ ] Support des sous-chaînes persistantes (slices partagées).  
- [ ] Accélération SIMD des opérations de recherche.  
- [ ] Compression adaptative des chaînes longues.  
- [ ] Support complet des locales et collations Unicode.  
- [ ] Visualisation du graphe Unicode dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
