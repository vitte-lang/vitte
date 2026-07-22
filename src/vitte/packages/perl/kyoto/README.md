# vitte-perl-kyoto

Version: `0.1.0`

Small assertion harness for embedded package tests. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Kyoto`

## Public API

- `suite`
- `assert_true`
- `assert_eq`
- `assert_deep`
- `run_suite`
- `summary`

## Check

```sh
perl -Ilib t/basic.t
```
