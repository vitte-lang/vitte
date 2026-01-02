# args_matrix

Test matrix for `vittec` / `vitte` CLI argument parsing and compatibility.

Goals:
- verify parsing of short/long flags, combined flags, flag values, repeated flags
- verify precedence (last-wins, or error) per spec
- validate help output stability (golden)
- validate error diagnostics (exit code + stderr)
- cross-platform normalization (paths, newlines)

Conventions:
- `cases/*.case.json` describes one invocation scenario.
- `golden/*` contains expected stdout/stderr snapshots.
- `runner/` provides minimal runner glue (used by test harness).
