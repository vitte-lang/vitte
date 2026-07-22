# vitte-perl-paris

Version: `0.1.0`

Command line flag parsing and usage rendering. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Paris`

## Public API

- `parse_args`
- `has_flag`
- `option`
- `positionals`
- `usage`
- `require_option`
- `merge_defaults`

## Check

```sh
perl -Ilib t/basic.t
```
