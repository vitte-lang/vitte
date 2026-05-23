# Vitte Grammar (Core Freeze 0.1)

This file freezes the parser-level core subset accepted by the seed compiler.

- `space <name>` required once per module.
- top-level declarations: `proc`, `const`, `form`, `pick`, `export *`.
- entry function shape: `proc main(args: list[string]) -> int`.
- blocks use `{ ... }` and must be non-empty when mandatory.
- imports use `use/from/import` module paths.

Any syntax outside this subset is experimental and not guaranteed stable.
