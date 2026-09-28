# MIR

- MIR is produced from functions and control-flow markers.
- Verifier enforces: terminators present, no `Unknown`, no orphan/invalid place/operand markers.
- O1 applies minimal text-level transforms then re-verifies MIR.
