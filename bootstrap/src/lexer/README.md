# bootstrap/src/lexer

The lexer converts bootstrap source text into a stable token stream with byte
offsets and line/column spans.

## Contract

- No dependency on `runtime/*`.
- Errors use `bootstrap/src/api/error.h`.
- Input is a caller-owned source buffer plus an optional source name.
- Tokens keep pointers into the original source buffer.
- Offsets are byte offsets.
- Lines and columns are 1-based.
- Whitespace and comments are skipped by default.
- The lexer can emit whitespace and comment tokens when enabled in options.

## Token Coverage

Current bootstrap token kinds include:

- identifiers
- integer literals
- string literals
- keywords: `space`, `use`, `as`, `proc`, `const`, `let`, `if`, `else`, `give`, `and`, `or`, `not`
- punctuation: `(` `)` `{` `}` `,` `:` `::` `;`
- operators: `=` `==` `!` `!=` `+` `-` `*` `/` `%` `&` `&&` `|` `||` `^` `<` `<=` `<<` `>` `>=` `>>` `->`
- `error`
- `eof`

## Strings And Numbers

Integer literals are decimal and unsigned at the lexical layer. A leading `-`
is emitted as a separate token.

String literals support these escapes:

- `\\`
- `\"`
- `\n`
- `\r`
- `\t`

Invalid escapes and unterminated strings produce `error` tokens and set the
lexer last error.

## Comments

Supported comments:

- line comments: `// ...`
- block comments: `/* ... */`

Unterminated block comments produce `error` tokens.

## API

Main entry points:

- `vitte_lexer_init`
- `vitte_lexer_next`
- `vitte_lexer_peek`
- `vitte_lexer_lex_all`
- `vitte_token_kind_name`

`vitte_lexer_peek` caches one token. `vitte_lexer_next` consumes it without
rescanning.

## Limits

- source size is bounded by `max_source_bytes`
- token storage for `vitte_lexer_lex_all` is caller-provided
- tab width defaults to `4`
- path separators for module/import parsing are tokenized as `::` and `/`

## Example

```c
vitte_lexer_t lexer;
vitte_token_t token;

vitte_lexer_init(&lexer, "main.vit", "proc main() { give 0 }", 0u, NULL);
while (vitte_lexer_next(&lexer, &token) == VITTE_STATUS_OK) {
    if (token.kind == VITTE_TOKEN_EOF) {
        break;
    }
}
```
