# Vitte Stdlib API Stability

Public stdlib modules listed in `src/vitte/stdlib/stdlib_modules.json` are
treated as stable contracts.

Compatibility rules:

- public names are not removed without a migration alias;
- public signatures are documented in the generated API index;
- each public module must have a test reference;
- each public module must have an example reference;
- public `panic` usage must be documented or guarded by validation.
