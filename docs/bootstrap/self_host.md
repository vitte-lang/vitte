# Self Host

Run the generation-chain audit with:

```sh
python3 tools/selfhost_completion_audit.py
```

It uses `toolchain/seed/vittec0.seed` through `bin/vittec0` to build
`src/vitte/compiler/main.vit`, then rebuilds the same compiler entry with the
generated compiler. Results are written to:

- `target/selfhost_completion/selfhost_completion.json`
- `target/reports/selfhost_completion.md`

Official completion requires:

```sh
python3 tools/selfhost_completion_audit.py --strict-complete
```

That command fails while generation hashes differ or any transition bridge is
present.
