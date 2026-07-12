# HIR

Path: `src/vitte/compiler/middle/hir`

## Purpose

Canonical semantic representation between the validated frontend AST and MIR.

## AST lowering contract

- Every `AstExprKind`, `AstStmtKind`, `AstItemKind` and `AstNominalMemberKind` has an explicit same-named HIR mapping in `lower_ast.vit`.
- Pattern and type syntax is normalized to canonical text on the enclosing HIR node. Their kind-name functions must remain exhaustive.
- `Unknown` is reserved for absent or unsupported syntax. `Invalid` expressions remain explicitly invalid and cannot produce a valid HIR expression.
- Child expressions and all statement branches, including catch branches, are lowered recursively.

`python3 tools/ast_hir_lowering_audit.py` derives coverage from the `pick` declarations and fails when a frontend variant lacks a lowering strategy. It runs as part of `make hir-lowering-test` and therefore the core language gates.
