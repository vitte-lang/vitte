# Lyon Design

Lyon keeps control flow data explicit. A caller should be able to inspect the
same value from a terminal test, a package registry gate, an installer doctor,
or a compiler integration without parsing free-form text.

Design rules:

- Results and options are plain hashes with stable `type` fields.
- Error values carry a code, message, context, path, and optional cause.
- Context key order is stable for deterministic diagnostics.
- Recovery functions never hide errors unless the caller gives a fallback.
- Reports summarize result streams without side effects.
- Public functions are available through the main module and through focused
  submodules.

The package intentionally has no non-core runtime dependency.
