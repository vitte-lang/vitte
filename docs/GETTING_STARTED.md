# Getting Started

This is the shortest path from a fresh checkout to a first successful Vitte program.

If you only read one repository doc after `README.md`, read this one.

## 1. Build The Compiler

### macOS

```sh
brew install llvm openssl@3 curl
OPENSSL_DIR=/opt/homebrew/opt/openssl@3 make build
```

### Linux (Ubuntu/Debian)

```sh
sudo apt-get update
sudo apt-get install -y clang libssl-dev libcurl4-openssl-dev
make build
```

Compiler binary:

```text
vitte
```

## 2. Verify The Toolchain

Run:

```sh
vitte check examples/first_project.vit
vitte build examples/first_project.vit
```

If both commands pass, your local setup is working.

Recommended first example:

- `examples/first_project.vit`

## 3. Read One Small Program First

This is the smallest useful example from the main README:

```vit
proc add(a: int, b: int) -> int {
  give a + b
}

entry main at core/app {
  let result: int = add(2, 3)
  give result
}
```

What to notice:

- `proc` defines a procedure
- `give` returns a value
- `entry` marks the program entrypoint
- Vitte prefers explicit structure over inference-heavy syntax

## 4. Use The Core Commands

```sh
vitte parse path/to/file.vit
vitte check path/to/file.vit
vitte build path/to/file.vit
```

- `parse`: syntax and structure only
- `check`: semantic validation without full build output
- `build`: compile the program

## 5. Bootstrap With Templates

Use `init` with an explicit template:

```sh
vitte init app-cli --template cli
vitte init app-service --template service
vitte init app-lib --template lib-native
```

List templates:

```sh
vitte init --list-templates
```

## 6. Understand Surface Stability

Vitte is experimental overall, but not every surface has the same expectation.

| Label | Meaning |
| --- | --- |
| `stable` | protected by an explicit gate or compatibility policy |
| `experimental` | implemented, but outside the protected contract |
| `internal` | not a public API or public language promise |

Important references:

- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `docs/LANGUAGE_CORE_SURFACE.md`

## 7. Choose Your Next Step

If you want to contribute safely:

```sh
make build
make parse
make hir-validate
make test
```

If you are changing language behavior or diagnostics, also run:

```sh
make core-language-gate
```

If you are validating release-facing core promises, also run:

```sh
make core-release-gate
```

For DX/adoption checks (init templates, frequent diagnostics autofix guidance, local completion latency, hello->prod KPI), run:

```sh
make dx-adoption
```

## 8. Where To Go Next

- `README.md` for the project overview
- `docs/FIRST_PROJECT.md` for the canonical first example
- `CONTRIBUTING.md` for contribution workflow
- `ROADMAP.md` for current priorities
- `docs/LANGUAGE_CORE.md` for core language policy
