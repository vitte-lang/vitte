# Vitte Stdlib API Stability

Public source modules currently live under `modules/`. The previous inventory path `src/vitte/stdlib/stdlib_modules.json` has been retired, and the generated API atlas still reflects an older source tree.

When updating a public module:

- document public names and signatures;
- keep a test reference for changed behavior;
- include an example where the module has an examples directory;
- document panic behavior or guard it with validation.

The generated API material should be refreshed from `modules/` before it is treated as the current public inventory.
