# Numbered Stages Retired

The historical stage1 bootstrap command and `bin/vittec1` artifact have been
removed. The active bootstrap starts from `toolchain/seed/vittec0.seed`, installs
`bin/vittec0`, and validates the real compiler entry at
`src/vitte/compiler/main.vit`.

Use:

```sh
make bootstrap-vitte-hard-gate
```
