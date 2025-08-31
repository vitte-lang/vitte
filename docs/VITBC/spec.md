# Spécification VITBC (Vitte ByteCode) — v2

> Conteneur binaire structuré, sections typées, CRC32, compression optionnelle (zstd/lz4).

## En-tête
| Champ           | Taille | Valeur         |
|-----------------|--------|----------------|
| MAGIC           | 4      | `VITB`         |
| FILE_VERSION    | 2      | `0x0002`       |
| FLAGS           | 2      | bitflags       |
| TOC_OFFSET      | 8      | offset table   |

## Sections usuelles
- `CODE` : bytecode VM
- `DATA` : blobs (zstd/lz4 si flag)
- `SYMB` : symboles/fonctions
- `META` : JSON compact optionnel (debug)

## Table des matières (TOC)
Entrées triées par id de section, `(kind, offset, len, crc32)`.

## CRC
CRC32 `IEEE` des **données** de la section.

## Pseudocode de lecture
```rust
read_header()?;
seek(TOC_OFFSET);
for ent in read_toc()? {
  map(ent.offset..ent.offset+ent.len);
  if flags & COMPRESSED { decompress(..) }
  verify_crc32(..);
}
```
