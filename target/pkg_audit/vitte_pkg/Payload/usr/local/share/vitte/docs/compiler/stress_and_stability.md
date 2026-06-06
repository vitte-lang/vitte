# 108/109 Stress And Long-Term Stability

## Compiler Stress Tests (Maximal)

Goal: test operational boundaries under extreme compiler workloads.

Run:

```sh
python3 tools/compiler_stress_maximal.py --all --report build/reports/stress.txt
```

or:

```sh
make stress-maximal
```

Covered stress families:

- 10k functions
- 100k tokens
- deeply nested expressions
- huge match/select
- many errors in one file

Measured and reported:

- wall time per stress run (phase proxy)
- max RSS memory (best effort)
- output binary size (when build succeeds)
- command/exit status per case

Output:

- `build/reports/stress.txt`

## Long-Term Stability Suite (Maximal)

Goal: prevent long-run regressions across correctness, reproducibility, and performance.

Run:

```sh
python3 tools/long_term_stability_suite.py
```

or:

```sh
make long-term-stability-suite
```

Suite includes:

- nightly full
- weekly stress
- release gate
- bootstrap gate
- fuzz gate
- benchmark gate
- deterministic gate

Reports:

- `build/reports/nightly.md`
- `build/reports/benchmarks.json`
- `build/reports/regressions.txt`

Failure policy:

- unstable diagnostics / deterministic gate failures -> fail
- gate command failures -> fail
- major performance degradation (>50% vs previous benchmark record) -> fail
- hard crash behavior in stress suite -> fail
