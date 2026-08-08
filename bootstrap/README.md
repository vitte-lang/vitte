# Vitte C17 Bootstrap

This directory contains the dependency-free C17 bootstrap compiler used to
materialize the first native Vitte binary.

Build it with:

```sh
make bootstrap-binary
```

or directly:

```sh
make -C bootstrap install
```

The resulting executable is `bin/vitte-bootstrap`. It supports `check`,
`emit-c`, `build`, `run`, `--help`, and `--version`.
