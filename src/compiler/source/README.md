# Native Vitte source manager

`source.vitte` implements all source-manager functions from
`src/bootstrap/source/source.h` using Vitte strings and arrays. It has no
`export extern proc` bindings and depends only on the native Unicode module.

The manager owns registered names, paths, source contents, and line-offset
indexes. Source IDs are stable, one-based, and bounded to the parser's 32-bit
file-ID range. Nonempty paths must be unique. Input source must be valid UTF-8.

Line indexing recognizes LF, CRLF, and lone CR. Locations use 1-based lines
and Unicode-codepoint columns; slices and spans use half-open byte offsets.
Validation and fingerprints avoid pointer values and allocator state.

The root `Makefile` still builds only the C bootstrap. It does not compile
`src/compiler/source/source.vitte` yet.
