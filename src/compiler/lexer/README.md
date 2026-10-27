# Native Vitte lexer

`lexer.vitte` implements the compiler lexer in Vitte. Its token kinds, public
function names, and helper queries match `src/bootstrap/lexer/lexer.h`; the
Vitte module has no `export extern proc` bindings.

## Contract

- Source offsets and token spans are half-open byte ranges.
- Lines and columns are 1-based; columns follow the bootstrap lexer's byte
  advancement rules, including its tab width of four.
- The lexer owns a Vitte string copy of its input and stores tokens in a Vitte
  array. Token lexemes are source slices.
- UTF-8 decoding and identifier classification use
  `compiler::unicode::unicode`.
- Whitespace and comments are skipped. Block comments may nest up to the
  configured depth.
- Number support includes binary, octal, decimal and hexadecimal integers,
  decimal and hexadecimal floats, and digit separators.
- String and character literals validate the bootstrap escape set.
- Operators use longest-match ordering.
- Token limits, validation, statistics and FNV fingerprints are implemented
  in Vitte.

The root `Makefile` still builds the C bootstrap under `src/bootstrap/` and
does not compile this module yet. The native lexer becomes the active compiler
lexer when the Vitte pipeline is wired into the bootstrap cutover.
