# vitte-perl-athens

Version: `0.1.0`

Temporary path planning and cleanup manifests. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Athens`

## Public API

- `temp_name`
- `temp_path`
- `cleanup_plan`
- `join_path`
- `basename`
- `dirname`
- `extension`

## Check

```sh
perl -Ilib t/basic.t
```
