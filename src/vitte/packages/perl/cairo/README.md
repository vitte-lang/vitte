# vitte-perl-cairo

Version: `0.1.0`

Stable scalar and record serialization helpers. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Cairo`

## Public API

- `encode_scalar`
- `decode_scalar`
- `encode_record`
- `decode_record`
- `encode_list`
- `decode_list`

## Check

```sh
perl -Ilib t/basic.t
```
