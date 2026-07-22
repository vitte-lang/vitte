# vitte-perl-lisbon

Version: `0.1.0`

Version parsing, ordering, and range checks. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Lisbon`

## Public API

- `parse_version`
- `format_version`
- `compare_version`
- `is_prerelease`
- `satisfies_min`
- `bump_major`
- `bump_minor`
- `bump_patch`

## Check

```sh
perl -Ilib t/basic.t
```
