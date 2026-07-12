# Legacy bootstrap IR fixtures

Path: `src/vitte/compiler/ir`

## Purpose

Compatibility fixtures retained for bootstrap smoke coverage. These modules
predate the production frontend, HIR, MIR, and backend IR trees.

## Notes

- Production AST/HIR/MIR code lives under `frontend/ast` and `middle`.
- The canonical backend contract is `backend/ir::IrUnit`.
- Production entry points must not import this directory.
