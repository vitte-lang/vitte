# vitte-perl-prague

Version: `0.1.0`

Configuration parsing, merging, and emission. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Prague`

## Public API

- `parse_config`
- `emit_config`
- `get_config`
- `set_config`
- `merge_config`
- `require_config`

## Check

```sh
perl -Ilib t/basic.t
```
