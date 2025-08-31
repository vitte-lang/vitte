# Stdlib (vitte-stdlib)

- `ids` : `Id<T>`, `IdGen`
- `arena` : `Arena<T>`, `IdMap<T, U>`
- `result` : `VResult`, macros `bail!`/`ensure!`
- `bytes` : helpers endian sans `std`
- `strutil` : mini utilitaires (`to_snake_case`, `to_lower_ascii`)
- `time` : trait `InstantLike` (impl std)

Features : `std` (défaut), `alloc-only`, `serde`, `small`, `index`, `errors`.
