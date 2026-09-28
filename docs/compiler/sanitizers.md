# Sanitizer Support (Maximal)

Goal: detect memory corruption, UB, and runtime/backend safety issues as early as possible.

## Profiles

- `debug`
- `debug-asan`
- `debug-ubsan`
- `debug-tsan`
- `release`
- `release-lto`

## Commands

```sh
vittec test --profile debug-asan
vittec test --profile debug-ubsan
vittec e2e --profile debug-asan
```

## Automated runner

```sh
make sanitizers-maximal
```

Expected logs:

- `build/logs/asan.txt`
- `build/logs/ubsan.txt`
- `build/logs/tsan.txt`

## Failing conditions

Fail CI/report if logs contain critical patterns:

- critical leaks
- invalid read/write
- undefined behavior
- double free
- use-after-free
- data race
