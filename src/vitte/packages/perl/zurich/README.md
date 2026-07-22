# vitte-perl-zurich

Version: `0.1.0`

Ordered maps, sets, grouping, and indexing helpers. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Zurich`

## Public API

- `ordered_map`
- `map_put`
- `map_get`
- `map_keys`
- `set`
- `set_has`
- `set_add`
- `group_by`
- `index_by`

## Check

```sh
perl -Ilib t/basic.t
```
