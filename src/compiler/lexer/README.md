# Native Vitte lexer

`lexer.vitte` is the native compiler lexer, implemented in Vitte. Its token
kinds, public function names and helper queries mirror
`src/bootstrap/lexer/lexer.h`; the module has no `export extern proc` bindings.

## Contract

- Source offsets and token spans are half-open byte ranges.
- Lines and columns are 1-based; columns follow the bootstrap lexer's byte
  advancement rules, including its tab width of four.
- The lexer stores tokens in a Vitte array and owns a terminated copy of each
  token lexeme. `reset` and `destroy` release those copies.
- UTF-8 decoding and identifier classification use
  `compiler::unicode::unicode`; identifiers follow the full Unicode 16.0.0
  XID_Start and XID_Continue tables, with underscore accepted at the start.
- A UTF-8 BOM is skipped at the beginning of a source file. It remains part of
  the source byte offsets, so the first token begins after the three BOM bytes.
- Whitespace and comments are skipped. Block comments may nest up to the
  configured depth.
- Number support includes binary, octal, decimal and hexadecimal integers,
  decimal and hexadecimal floats, digit separators, and checked unsigned
  64-bit integer range validation.
- String and character literals validate the bootstrap escape set.
- Operators use longest-match ordering.
- Token limits, validation, statistics and FNV fingerprints are implemented
  in Vitte.

The native frontend pipeline calls this lexer directly and converts its token
records for the parser. The C lexer under `src/bootstrap/` remains the seed's
reference implementation; it is not the active lexer inside the native
frontend. The root build uses that seed to compile the Vitte frontend modules.

The lexer is intentionally a whole-source pass: `vitte_lexer_run` emits the
complete token array, including EOF. Call `vitte_lexer_reset` to reuse a lexer
for another source, or `vitte_lexer_destroy` when the token stream is no longer
needed.
