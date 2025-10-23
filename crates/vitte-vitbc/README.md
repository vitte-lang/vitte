

# vitte-vitbc

Système **de format binaire compilé (.vitbc)** pour le langage Vitte.  
`vitte-vitbc` définit la structure, la sérialisation, la validation et les outils d’inspection du format binaire intermédiaire utilisé par le compilateur et le runtime.

---

## Objectifs

- Offrir une **spécification binaire stable, compacte et extensible** pour les fichiers `.vitbc`.  
- Permettre la lecture, l’écriture et la validation des artefacts compilés.  
- Assurer la compatibilité entre versions du compilateur et du runtime.  
- Intégration directe avec `vitte-compiler`, `vitte-runtime`, `vitte-analyzer`, `vitte-inspect` et `vitte-tools`.  
- Fournir une API haut niveau pour manipulation et introspection du bytecode.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `header`      | Définition du header de fichier (`magic`, version, taille) |
| `section`     | Gestion des sections (code, données, imports, exports) |
| `reader`      | Lecture séquentielle et aléatoire du flux binaire |
| `writer`      | Sérialisation et génération des fichiers `.vitbc` |
| `validate`    | Vérification d’intégrité et de compatibilité du format |
| `inspect`     | Extraction et visualisation des métadonnées internes |
| `tests`       | Couverture complète des cas de parsing et génération |

---

## Spécification du format `.vitbc`

### Structure générale

| Champ          | Taille (octets) | Description |
|----------------|----------------|--------------|
| Magic Header   | 4               | Identifiant `VBC1` |
| Version        | 2               | Version majeure/mineure du format |
| Flags          | 2               | Indicateurs (endianness, compression, debug) |
| Section Count  | 4               | Nombre total de sections |
| Offset Table   | variable        | Table des offsets vers chaque section |
| Sections       | variable        | Données brutes par section |

### Types de sections

- **CODE** : instructions machine/intermédiaire.  
- **DATA** : constantes et ressources intégrées.  
- **IMPORTS** : dépendances externes et symboles requis.  
- **EXPORTS** : symboles accessibles depuis d’autres modules.  
- **META** : métadonnées, hash, build info.  
- **DEBUG** : symboles et mappages source.

---

## Exemple d’utilisation

### Lecture d’un fichier `.vitbc`

```rust
use vitte_vitbc::VitbcFile;

fn main() -> anyhow::Result<()> {
    let file = VitbcFile::open("target/main.vitbc")?;
    println!("Version: {}", file.header.version);
    println!("Sections: {:?}", file.sections());
    Ok(())
}
```

### Écriture d’un fichier compilé

```rust
use vitte_vitbc::{VitbcFile, Section};

fn main() -> anyhow::Result<()> {
    let mut file = VitbcFile::new();
    file.add_section(Section::code(vec![0xDE, 0xAD, 0xBE, 0xEF]));
    file.add_section(Section::meta(b"compiler=Vitte 0.1.0".to_vec()));
    file.save("target/demo.vitbc")?;
    Ok(())
}
```

---

## Intégration

- `vitte-compiler` : production directe des fichiers `.vitbc`.  
- `vitte-runtime` : chargement et exécution des artefacts compilés.  
- `vitte-analyzer` : inspection et visualisation des sections.  
- `vitte-tools` : validation et signature des binaires.  
- `vitte-docgen` : génération automatique des schémas du format.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-hash = { path = "../vitte-hash", version = "0.1.0" }

byteorder = "1.5"
crc32fast = "1.4"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
blake3 = "1.5"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `lz4_flex` pour compression, `binrw` pour parsing automatique, `hex` pour dump formaté.

---

## Tests

```bash
cargo test -p vitte-vitbc
```

- Tests de parsing et round-trip (lecture/écriture).  
- Tests de validation des headers et offsets.  
- Tests de compatibilité entre versions du format.  
- Tests de performance sur fichiers volumineux.

---

## Outils d’inspection

```bash
vitte inspect --header --meta main.vitbc
vitte inspect --sections --size main.vitbc
vitte inspect --hexdump main.vitbc
vitte inspect --json main.vitbc
```

Ces commandes permettent d’explorer les métadonnées, la structure et les tailles de sections.  
Intégré au module `vitte-tools` pour automatisation CI/CD.

---

## Roadmap

- [ ] Spécification complète du format `.vitbc` v2.0.  
- [ ] Compression transparente LZ4/ZSTD.  
- [ ] Signatures intégrées (`vitte-verify`).  
- [ ] Compatibilité croisée pour architectures (x86_64, ARM64, RV64, WASM).  
- [ ] Visualisation binaire dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau