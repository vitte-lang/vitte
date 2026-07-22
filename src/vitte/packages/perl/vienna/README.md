# vitte-perl-vienna

Version: `0.1.0`

Numeric helpers for statistics and bounded arithmetic. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Vienna`

## Public API

- `sum`
- `mean`
- `median`
- `minmax`
- `clamp`
- `lerp`
- `percentile`
- `variance`

## Check

```sh
perl -Ilib t/basic.t
```
