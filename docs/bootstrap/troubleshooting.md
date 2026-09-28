# Troubleshooting

Common failures:

- Missing `bin/vittec0`: run `make bootstrap-seed`; installation fails if the
  manifest-pinned seed is invalid.
- Checksum mismatch: inspect the rotation with `make seed-rotation-report` and
  update the manifest only for an intentional, reviewed seed change.
- Native snapshot drift: update the relevant IR, diagnostic, or shell snapshot
  and rerun `make bootstrap-native-snapshots` without update mode.
- Self-host completion reports `transition`: inspect generation hashes and
  bridge markers in `target/selfhost_completion/selfhost_completion.json`.
