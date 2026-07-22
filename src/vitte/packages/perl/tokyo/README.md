# vitte-perl-tokyo

Version: `0.1.0`

Deterministic task queue and small runtime helpers. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Tokyo`

## Public API

- `runtime`
- `spawn`
- `run_next`
- `block_on`
- `join_all`
- `defer`
- `task_count`

## Check

```sh
perl -Ilib t/basic.t
```
