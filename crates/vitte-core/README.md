# vitte-core

Primitives partagées pour l’écosystème Vitte : positions source, identifiants textuels,
helpers bytecode (lecteur/écrivain, chunk, assembleur/désassembleur minimal),
CRC32 sans table et erreurs communes.

Le crate sert de socle aux autres modules (`vitte-expander`, `vitte-cli`, etc.) en
fournissant un ensemble léger et `no_std`-friendly de types utilitaires.

---

## Fonctionnalités principales

- Internage global thread-safe via `symbol::intern` (`Symbol`, `Symbol::as_str`).
- Types de positionnement : `SourceId`, `Pos`, `Span`, `Spanned<T>`.
- IO mémoire en little-endian (`ByteWriter`, `ByteReader`) et calcul CRC32.
- Modèle de chunk bytecode avec assembleur/disassembleur de compatibilité.
- Erreurs partagées `CoreError` / `CoreResult`.

---

## Features Cargo

| Feature       | Description |
|---------------|-------------|
| `std` *(défaut)* | Active `std::error::Error`, `bincode` et `thiserror`. |
| `alloc-only`  | Compile sans `std` (requires `alloc` seulement). |
| `serde`       | Derives `Serialize`/`Deserialize` pour les structures publiques. |
| `postcard`    | Ajoute la sérialisation binaire `postcard` (no_std friendly). |
| `zstd`        | Active la compression optionnelle Zstandard. |

Activer le mode `no_std` :

```bash
cargo check -p vitte-core --no-default-features --features alloc-only
```

Avec sérialisation :

```bash
cargo check -p vitte-core \
  --no-default-features \
  --features alloc-only,serde,postcard
```

---

## Exemple rapide

```rust
use vitte_core::{
    bytecode::asm,
    ByteReader, ByteWriter, CoreResult, SectionTag,
};

fn roundtrip() -> CoreResult<()> {
    // Assemble un petit chunk.
    let chunk = asm::assemble(r#"
        CONST "hello"
        LOAD 0
        PRINT
        RETURN
    "#)?;

    // Encode + relit le chunk pour vérifier l’aller-retour binaire.
    let bytes = chunk.to_bytes();
    let decoded = vitte_core::bytecode::chunk::Chunk::from_bytes(&bytes)?;

    assert_eq!(chunk.compute_hash(), decoded.compute_hash());

    // Helpers I/O bruts.
    let mut w = ByteWriter::new();
    w.write_tag(SectionTag::CODE);
    let mut r = ByteReader::new(w.as_slice());
    assert_eq!(r.read_tag()?, SectionTag::CODE);
    Ok(())
}
```

---

## Tests

```bash
cargo test -p vitte-core
```

Pour valider le profil embarqué :

```bash
cargo check -p vitte-core --no-default-features --features alloc-only,serde,postcard
```

---

## Roadmap

- [x] Support complet `no_std`.  
- [x] Pool global de symboles thread-safe.  
- [ ] Benchmarks mémoire et hashing.  
- [ ] Compatibilité WebAssembly.  
- [ ] Documentation API publique détaillée.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
