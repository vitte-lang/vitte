# vitte-perl-lima

Version: `0.1.0`

Iterator style transforms for arrays and small streams. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Lima`

## Public API

- `map_list`
- `filter_list`
- `fold_list`
- `chunks`
- `zip`
- `take`
- `unique`
- `flatten`

## Check

```sh
perl -Ilib t/basic.t
```
