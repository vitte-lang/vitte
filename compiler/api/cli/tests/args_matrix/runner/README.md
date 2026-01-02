# runner

Glue code for running cases.

In the real repo, this runner would:
- spawn `vittec` with argv + env + cwd
- capture stdout/stderr (binary-safe)
- normalize output (paths/newlines/ansi) for golden compare
- emit JSON/JUnit report
