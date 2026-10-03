# Vitte runtime

This directory is the source-level runtime contract distributed with Vitte.

`runtime.vit` provides the stable runtime identity, API version, readiness
check, and contract validation surface. Compiler-generated runtime support may
be extended behind this contract while keeping the installed path stable:

```text
$VITTE_ROOT/runtime/runtime.vit
```

The installer packages this directory under `share/vitte/runtime` for every
supported platform.
