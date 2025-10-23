

# vitte-verify

Système **de vérification, validation et intégrité** pour l’écosystème Vitte.  
`vitte-verify` assure la cohérence, la sécurité et la reproductibilité des modules, binaires, signatures et dépendances utilisés dans le langage et ses outils.

---

## Objectifs

- Offrir une **infrastructure complète de vérification des builds et dépendances**.  
- Garantir l’intégrité des fichiers et artefacts binaires.  
- Intégration directe avec `vitte-build`, `vitte-compiler`, `vitte-tools` et `vitte-updater`.  
- Supporter les algorithmes de hachage, signature et checksum multiples.  
- Détecter toute modification ou incohérence entre les sources et les métadonnées.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `hash`        | Calcul et validation de hash (SHA-256, BLAKE3, XXHash) |
| `signature`   | Vérification cryptographique des signatures GPG/X.509 |
| `manifest`    | Lecture et validation des fichiers `vitte.toml` et métadonnées |
| `integrity`   | Vérification des artefacts compilés et caches |
| `report`      | Génération de rapports de conformité JSON/HTML |
| `tests`       | Scénarios de validation et de performance |

---

## Exemple d’utilisation

```rust
use vitte_verify::{Verifier, Policy};

fn main() -> anyhow::Result<()> {
    let verifier = Verifier::new(Policy::Strict);
    verifier.verify_file("target/release/vitte")?;
    println!("Build vérifié avec succès ✅");
    Ok(())
}
```

### Exemple CLI

```bash
vitte verify --manifest vitte.toml --strict
```

---

## Intégration

- `vitte-build` : validation post-compilation des artefacts.  
- `vitte-tools` : vérification des paquets et archives.  
- `vitte-updater` : contrôle des mises à jour téléchargées.  
- `vitte-compiler` : validation des modules compilés.  
- `vitte-cloud` : intégrité des synchronisations distantes.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-hash = { path = "../vitte-hash", version = "0.1.0" }

blake3 = "1.5"
sha2 = "0.10"
xxhash-rust = "0.8"
gpgme = { version = "0.12", optional = true }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `openssl` pour signature X.509, `flate2` pour vérification de contenu compressé.

---

## Tests

```bash
cargo test -p vitte-verify
```

- Tests de hachage et cohérence binaire.  
- Tests de vérification de signatures valides/invalides.  
- Tests de compatibilité entre politiques de sécurité.  
- Benchmarks de validation multi-fichier.

---

## Roadmap

- [ ] Intégration avec `vitte-cloud` pour vérification distante.  
- [ ] Signatures reproductibles pour builds déterministes.  
- [ ] Mode audit complet multi-crates.  
- [ ] Support des attestations SBOM et provenance (`vitte-sbom`).  
- [ ] Interface graphique dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau