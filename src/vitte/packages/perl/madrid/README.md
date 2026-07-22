# vitte-perl-madrid

Version: `0.1.0`

Identifier generation, validation, normalization, and parsing. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Madrid`

## Public API

- `nil_id`
- `is_nil`
- `normalize_id`
- `is_id`
- `format_id`
- `new_counter_id`
- `short_id`

## Check

```sh
perl -Ilib t/basic.t
```
