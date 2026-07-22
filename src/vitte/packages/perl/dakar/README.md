# vitte-perl-dakar

Version: `0.1.0`

Structured logging with levels, fields, and in-memory sinks. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Dakar`

## Public API

- `logger`
- `log_event`
- `debug`
- `info`
- `warn`
- `error`
- `entries`
- `with_field`
- `filter_level`

## Check

```sh
perl -Ilib t/basic.t
```
