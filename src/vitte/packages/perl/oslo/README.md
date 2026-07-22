# vitte-perl-oslo

Version: `0.1.0`

Duration, clock, and timestamp formatting helpers. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Oslo`

## Public API

- `duration`
- `seconds`
- `millis`
- `add_duration`
- `compare_duration`
- `format_duration`
- `epoch_parts`

## Check

```sh
perl -Ilib t/basic.t
```
