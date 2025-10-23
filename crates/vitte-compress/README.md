

# vitte-compress

Bibliothèque **de compression et décompression** utilisée dans l’écosystème Vitte.  
`vitte-compress` fournit des algorithmes performants et intégrés pour la réduction, l’archivage et le transport des artefacts binaires et métadonnées.

---

## Objectifs

- Offrir des API unifiées pour la compression et la décompression.  
- Supporter plusieurs formats : **LZ4**, **Zstd**, **Deflate**, **Gzip**, **Brotli**.  
- Gestion des flux **synchrone et asynchrone**.  
- Détection automatique du format via **magic bytes**.  
- Intégration directe avec `vitte-cache`, `vitte-build` et `vitte-docgen`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `core`        | Interface générique de compression et traits `Compressor` / `Decompressor` |
| `lz4`         | Implémentation rapide LZ4 / LZ4F |
| `zstd`        | Compression haute performance Zstandard |
| `gzip`        | Compatibilité Deflate/Gzip |
| `brotli`      | Compression optimisée pour le web |
| `detect`      | Détection automatique du format et du niveau |
| `stream`      | API de flux et buffers compressés |
| `async`       | Adaptateurs asynchrones basés sur Tokio |
| `tests`       | Tests de cohérence, performance et intégration |

---

## Exemple d’utilisation

```rust
use vitte_compress::{Compressor, Decompressor, Format};

fn main() -> anyhow::Result<()> {
    let data = b"Hello, world!";

    // Compression Zstd
    let compressed = Format::Zstd.compress(data)?;

    // Décompression automatique
    let decompressed = Format::detect(&compressed)?.decompress(&compressed)?;

    assert_eq!(data.to_vec(), decompressed);
    Ok(())
}
```

---

## Intégration

- `vitte-cache` : compression des artefacts et métadonnées.  
- `vitte-build` : compression des binaires de build.  
- `vitte-docgen` : minification des fichiers HTML/JSON générés.  
- `vitte-analyzer` : réduction des graphes sémantiques volumineux.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

lz4_flex = "1.6"
zstd = "0.13"
flate2 = { version = "1.0", features = ["zlib"] }
brotli = "6.0"
async-compression = { version = "0.4", features = ["tokio", "zstd", "gzip", "brotli"] }
anyhow = "1"
thiserror = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
``` 

> Optionnel : `xz2` pour LZMA/XZ, `bzip2` pour compatibilité rétro.

---

## Tests

```bash
cargo test -p vitte-compress
```

- Tests de compatibilité entre formats.  
- Vérification de la détection automatique.  
- Benchmarks comparatifs (taille et vitesse).  
- Tests asynchrones sur flux Tokio.

---

## Roadmap

- [ ] Support du format XZ/LZMA.  
- [ ] Compression adaptative multi-thread.  
- [ ] Cache compressé à granularité fine.  
- [ ] Intégration avec `vitte-analyzer` pour graphes compressés.  
- [ ] Benchmarks automatiques en CI.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau