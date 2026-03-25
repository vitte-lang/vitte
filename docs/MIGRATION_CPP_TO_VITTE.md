# Migration C++ -> Vitte

This migration guide is production-oriented and evidence-first.

## 1. Pick A Migration Shape

Start with one of these:

1. CLI replacement (`vitte init --template cli`)
2. Service edge component (`vitte init --template service`)
3. Native library boundary (`vitte init --template lib-native`)

Do not migrate the full codebase in one step.

## 2. Stabilize ABI Boundaries

For mixed C/C++ and Vitte stacks:

- keep C ABI stable and versioned
- generate/update interop headers
- run ABI compatibility checks

Commands:

```sh
make interop-headers-gen
make interop-headers-check
make interop-abi-matrix
```

## 3. Port Hot Paths Last

Migration order:

1. orchestration and glue layers
2. request/response boundaries
3. core algorithms only after profiling evidence

Use `make runtime-native-bench` and `make runtime-native-pgo-bench` for proof.

## 4. Replace Build/CI Incrementally

Map common C++ workflow to Vitte:

- `make build` -> `make build-fast` for local iteration
- sanitizer/perf jobs -> benchmark gates + release doctor
- ad hoc release notes -> `make release-proof-notes`

## 5. Gate Before Cutover

Minimum cutover checklist:

```sh
make core-release-gate
make dx-adoption
make public-benchmark-dashboard
make release-proof-notes
```

Only switch traffic after KPIs and compatibility checks are green.
