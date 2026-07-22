# vitte-perl-delhi

Version: `0.1.0`

Text templates, interpolation, indentation, and wrapping. This package is intentionally small, dependency-light,
and usable through `PERL5LIB` after extraction from the local Vitte registry.

## Module

`Vitte::City::Delhi`

## Public API

- `render_template`
- `indent`
- `dedent`
- `wrap_words`
- `slugify`
- `lines`

## Check

```sh
perl -Ilib t/basic.t
```
