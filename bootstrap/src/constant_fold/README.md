# Bootstrap Constant Fold

The constant fold layer evaluates bootstrap AST expressions into typed constant values. It is intentionally independent from `sema`, `type`, `scope`, and `symbol` while those layers are still being built.

Invariants:
- No dependency on `runtime/*`.
- Errors use `bootstrap/src/api/error.h`.
- AST nodes are not mutated.
- Evaluation is deterministic.
- Recursion depth is bounded.
- Integer overflow and division by zero are detected before arithmetic is executed.
- User symbols are not resolved until scope/symbol tables exist.

Supported AST expressions:
- integer literal
- string literal
- identifiers for builtin constants `true` and `false`
- binary expressions over folded operands

Supported operators:
- integer arithmetic: `+`, `-`, `*`, `/`, `%`
- integer comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- boolean logic/equality: `&&`, `||`, `==`, `!=`
- string equality: `==`, `!=`

Unsupported semantics:
- user identifiers
- calls
- string concatenation
- unsupported AST node kinds

Stats:
- expressions seen
- expressions folded
- builtin constants resolved
- overflow count
- divide-by-zero count
- unsupported count
- error count

Limitations:
- Results are values only; no AST rewrite pass is performed yet.
- All integer literals fold as signed 64-bit values.
- String equality uses byte string comparison on the literal text.
