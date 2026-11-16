# src/unicode

Category: Unicode.

Purpose: strict UTF-8 handling and version-pinned Unicode identifier rules.

Identifier start and continuation use the Unicode 16.0.0 XID properties, with
underscore accepted at the start.
The native Vitte lookup module is generated from the bootstrap's checked-in
`unicode_xid_table.h` with `python3 scripts/generate-unicode-vitte-xid.py`.
Keep both tables synchronized when the bootstrap data is deliberately updated.
Identifier spelling is compared as UTF-8 bytes; Vitte does not normalize
canonically equivalent Unicode spellings.

The lexer ignores a UTF-8 BOM only at the beginning of a source file. Token
spans remain byte offsets, and line and column counters keep their documented
bootstrap-compatible behavior.

The native frontend imports this module directly; its generated XID data is
kept in sync with the bootstrap table by the script above.
