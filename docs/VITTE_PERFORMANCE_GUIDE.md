# Vitte Performance Guide

This guide is focused on practical wins you can measure quickly.

## 1. Start With Measurements

Run baseline reports first:

```sh
make perf-baseline
make runtime-native-bench
make runtime-native-pgo-bench
```

Use generated artifacts under `target/reports/competitive/`.

## 2. Choose The Right Build Profile

- local dev loop: `make build-fast`
- optimized build: `make build-release`
- optimized + profile-guided: `make build-pgo-generate` then `make build-pgo-use`

PGO should be validated with representative training runs before publication claims.

## 3. Optimize DX Feedback Loop

Run:

```sh
make lsp-completion-gate
make dx-hello-prod-gate
```

Targets:

- completion p95 `< 50 ms`
- hello->prod reduction `>= 40%`

## 4. Keep Diagnostics Actionable

Run:

```sh
make diag-autofix-frequent
```

The goal is to keep frequent errors paired with explicit fix guidance.

## 5. Prevent Regressions

Before release publication:

```sh
make core-release-gate
make dx-adoption
make public-benchmark-dashboard
```

Treat any failed KPI as a blocker or documented limitation.
