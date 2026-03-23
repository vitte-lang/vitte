# First Project

The canonical first repository example is:

- `examples/first_project.vit`

Use it when you want one small program that is more representative than a pure syntax sample.

## Run It

```sh
bin/vitte check examples/first_project.vit
bin/vitte build examples/first_project.vit
```

## Why This Example

This example is intentionally small, but it still exercises:

- explicit procedures
- typed parameters and returns
- loops
- conditionals
- an entrypoint

It is a better first stop than a syntax showcase when you want to understand normal Vitte structure.

## When To Use Other Examples

- use `examples/syntax_features.vit` when you want the narrowest syntax smoke test
- use `examples/core_project_*.vit` when you want larger core-language exercises
- use embedded or package-specific examples when you are testing a narrower subsystem
