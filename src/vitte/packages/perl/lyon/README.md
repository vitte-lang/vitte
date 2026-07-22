# vitte-perl-lyon

Version: `0.1.0`

Result values, errors, context chains, and recovery helpers. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Lyon`

## Public API

- `ok`
- `err`
- `is_ok`
- `is_err`
- `value`
- `error`
- `map_ok`
- `map_err`
- `and_then`
- `unwrap_or`

## Check

```sh
perl -Ilib t/basic.t
```
