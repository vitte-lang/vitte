# session

Path: `src/vitte/compiler/infrastructure/session`

## Purpose

Compiler session infrastructure: source manager, diagnostics, logger, ICE boundary.

## Notes

- Centralize cross-cutting state here.
- Keep render_text/render_json stable for tooling.
- Avoid hidden defaults for critical session data.
