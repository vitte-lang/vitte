# vitte-encoding

Bibliothèque **d’encodage et de détection de jeux de caractères** pour l’écosystème Vitte.

`vitte-encoding` fournit un moteur unifié pour la détection, la conversion et la validation des encodages texte. Il s’intègre avec les modules `vitte-io`, `vitte-core` et `vitte-analyzer` afin de garantir un traitement robuste des données textuelles dans tous les environnements.

---

## Objectifs

- Détection automatique d’encodage (UTF‑8, UTF‑16, ISO‑8859‑x, Shift‑JIS, etc.).
- Conversion transparente entre encodages.
- Gestion des erreurs et des séquences invalides.
- Intégration directe avec le pipeline I/O de Vitte.
- API sécurisée, asynchrone et extensible.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `detect`       | Détection heuristique d’encodage |
| `convert`      | Transcodage vers et depuis UTF‑8 |
| `validate`     | Vérification des séquences valides |
| `stream`       | Conversion en flux et buffers |
| `analyzer`     | Statistiques et heuristiques linguistiques |
| `ffi`          | API C pour intégration externe |
| `tests`        | Cas multi-langues et stress tests |

---

## Exemple d’utilisation

```rust
use vitte_encoding::{detect, convert};

fn main() -> anyhow::Result<()> {
    let bytes = std::fs::read("data.txt")?;
    let enc = detect(&bytes)?;
    let text = convert(&bytes, &enc, "utf-8")?;
    println!("Encodage détecté : {enc}");
    Ok(())
}
```

---

## Détection

Basée sur :
- Fréquences d’octets et motifs BOM.
- Tables de probabilité pour UTF‑8, ISO‑8859‑1/15, CP1252.
- Modèles trigrammes pour CJK (chinois, japonais, coréen).
- Intégration optionnelle avec ICU.

---

## Conversion

Supporte :
- `UTF-8`, `UTF-16LE/BE`, `UTF-32`.
- `ISO-8859-x`, `Windows-125x`.
- `Shift-JIS`, `EUC-JP`, `KOI8-R`, `MacRoman`.

Fonctions :
```rust
convert(bytes, "iso-8859-15", "utf-8");
convert_stream(reader, "windows-1252", "utf-8");
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
encoding_rs = "0.8"
chardetng = "0.1"
memchr = "2.7"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-encoding
```

- Tests multi-langues (latin, cyrillique, CJK).
- Vérification de précision de détection.
- Validation des conversions et round-trip.

---

## Roadmap

- [ ] Détection linguistique avancée (fréquences n‑gram).
- [ ] Intégration complète ICU.
- [ ] Support streaming asynchrone complet.
- [ ] Benchmarks multi‑threads.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
