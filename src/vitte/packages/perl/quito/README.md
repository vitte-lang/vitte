# vitte-perl-quito

Version: `0.1.0`

Seeded pseudo-random numbers, ranges, and selection. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Quito`

## Public API

- `seeded`
- `next_u32`
- `range`
- `choice`
- `shuffle`
- `bytes_random`

## Check

```sh
perl -Ilib t/basic.t
```
