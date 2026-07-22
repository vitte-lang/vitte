# vitte-perl-berlin

Version: `0.1.0`

Byte buffer and binary packing helpers. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Berlin`

## Public API

- `buffer`
- `from_string`
- `append`
- `length_of`
- `slice`
- `to_hex`
- `from_hex`
- `concat_buffers`

## Check

```sh
perl -Ilib t/basic.t
```
