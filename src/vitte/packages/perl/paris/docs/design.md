# Paris Design

Paris keeps command-line parsing deterministic and side-effect free.

Design rules:

- Parsed arguments are plain hashes with stable keys.
- Every option name is normalized by removing leading dashes and replacing
  dashes with underscores.
- Repeated options retain every value and expose the last value as the default
  reader result.
- `--` switches the parser into literal positional mode.
- Validators return structured hashes instead of printing directly.
- Shell quoting is conservative and deterministic.

The module has no non-core runtime dependency.
