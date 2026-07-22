# vitte-perl-seoul

Version: `0.1.0`

Pattern matching helpers for text, captures, and replacements. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Seoul`

## Public API

- `is_match`
- `find_all`
- `captures`
- `replace_all`
- `escape_pattern`
- `split_pattern`

## Check

```sh
perl -Ilib t/basic.t
```
