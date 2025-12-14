# Regex Plugin (Vitte)

Skeleton plugin layout for a regex engine usable by end-users.

- API layer: `api/`
- Engine layer: `engine/` (parser, AST/HIR, NFA/DFA, optimizations)
- Runtime: `runtime/` (matcher, input adapters, limits)
- Data: `data/` (optional generated tables)
- Tests / benches included as stubs.
