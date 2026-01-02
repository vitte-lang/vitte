# Bench Data

This directory hosts **benchmark and fuzz fixture data** used by the Vitte compiler/tooling CI and performance suites.

## Layout

- `seeds/` — hand-curated Vitte programs used as seed corpora (smoke, formatter, sema).
- `fuzz/` — fuzz-oriented corpora and long-run stress inputs.
- `generated/` — generated fixtures (not committed by default; produced by generators).
- `generators/` — deterministic Python generators (`gen_*.py`) + drivers.

> Note: Some repositories commit a small subset of `generated/` for stability. In Vitte, the recommended workflow is to regenerate deterministically from `generators/`.

## Deterministic generation

The generator suite is designed to be deterministic for a given `--seed` and argument set.

Common entrypoints:

- `generators/run_all.sh` — convenience wrapper (POSIX).
- `generators/refresh_all.py` — orchestrator that runs generators from `manifest.json`.
- `generators/run_ci.sh` / `generators/run_ci.ps1` — CI preset regen + validate.
- `generators/run_large.sh` / `generators/run_large.ps1` — large/stress dataset.
- `generators/refresh_fuzz.py` — fuzz-heavy subset with campaign support.

Typical commands:

```sh
cd compiler/bench/src/data/generators

# regenerate default dataset
./run_all.sh --out ../generated --seed 0 --count 50 --stress 2 --emit-md

# CI preset
./run_ci.sh --python python3 --preset ./presets/ci.json --generated ../generated

# fuzz subset
python3 ./refresh_fuzz.py --out ../generated --seed 0 --count 200 --stress 3

# validate
python3 ./validate_dataset.py --root ../generated --strict
```

## What to commit

Recommended policy:

- Commit **seeds** (`seeds/`) and **generator code** (`generators/`).
- Do **not** commit `generated/` unless you intentionally snapshot a baseline for reproducibility.

## Contracts

Each generator writes:

- `<OUT>/<generator>/index.json` — machine-readable index (includes `contract` name/version)
- `<OUT>/<generator>/README.md` — human-readable description
- `<OUT>/<generator>/sets/<id>_<name>/...` — case directories (inputs + expected outputs)

Consumers should treat `index.json` as the authoritative discovery mechanism.

## Security

Generators may process large inputs and tricky Unicode sequences.

- See `generators/SECURITY.md` for the threat model and hardening rules.

## License

Part of the Vitte repository; see repository root license.
