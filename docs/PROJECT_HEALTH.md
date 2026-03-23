# Project Health

This page is the short human-readable health summary for Vitte.

It should stay honest, compact, and less aspirational than marketing copy.

## Current Summary

| Area | Status | Notes |
| --- | --- | --- |
| Core language contract | yellow | bounded and documented, but still intentionally narrow |
| Compiler/toolchain | yellow | active and usable for contributors, still experimental overall |
| Package governance | green | ownership and package-shape discipline are a visible strength |
| CI / regression gates | green | repo has substantial gating and snapshot infrastructure |
| Public release story | yellow | improving, but still lighter than a stable language ecosystem |
| Platform breadth | yellow | Linux and macOS are the clearest paths; Windows remains experimental |
| VitteOS track | yellow | direction exists, but milestones are still planned rather than mature |

## Credible Platforms Today

Most credible paths based on repository docs and workflows:

- Linux (Ubuntu/Debian)
- macOS
- WSL2 for Windows users

Experimental path:

- native Windows support

## Protected Surface

The strongest explicit public contract currently centers on:

- the documented language core
- core language diagnostics under explicit gates
- the main `parse / check / build` command path

See:

- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `docs/PUBLIC_SURFACE.md`

## Strong Areas

- explicit language maturity vocabulary
- documented core language boundaries
- substantial CI and snapshot gating
- visible governance conventions for packages
- serious treatment of diagnostics and deterministic behavior

## Risk Areas

- broad ecosystem expectations would still be premature
- some surfaces are documented before they are fully maturity-classified
- release expectations are easier to infer than to read directly unless docs are kept synchronized
- repository breadth can make the project look more stable or more unified than some areas really are

## Maintenance Rule

If a high-level project status changes materially, update this page in the same PR that changed the status.
