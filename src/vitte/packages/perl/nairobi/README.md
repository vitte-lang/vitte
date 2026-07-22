# vitte-perl-nairobi

Version: `0.1.0`

Environment, path, and process context helpers. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Nairobi`

## Public API

- `env_get`
- `env_has`
- `path_list`
- `path_join`
- `path_normalize`
- `with_env`
- `program_name`

## Check

```sh
perl -Ilib t/basic.t
```
