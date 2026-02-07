# Contributing to Vitte

Thanks for your interest in contributing.

## Prerequisites

- `clang` / `clang++`
- `openssl` + `curl`

## Quick Build

```sh
make build
```

## Tests

```sh
make parse
make hir-validate
make negative-tests
make test
```

## Code Style

- C/C++: follow `.clang-format`
- Vitte: 2‑space indentation

## Recommended Workflow

1. Create a branch `codex/...`
2. Open a PR with a clear description
3. Add tests if you change syntax or compiler behavior

## Good Contribution Areas

- Bug fixes
- Compiler improvements
- Documentation and examples
- Tooling / editor integrations

## Communication

If you’re unsure, open an issue with:
- context
- a minimal example
- expected behavior

---

## FAQ

**Do I need tests for docs changes?**  
Not required, but add examples when possible.

**Can I refactor large modules?**  
Yes, but please explain the rationale in the PR.
