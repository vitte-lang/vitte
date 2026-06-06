# Week 7-8 - Fix Plan

## Critical/Major closure plan
1. Reproduce issue with minimal case.
2. Patch in stdlib/compiler module.
3. Add or update regression fixture.
4. Re-run targeted checks.
5. Mark ticket closed with evidence.

## Ticket -> patch mapping
- BETA-101 -> `src/vitte/stdlib/async/executor.vitl` (ready queue behavior + scheduling path)
- BETA-102 -> `src/vitte/stdlib/ffi/ffi.vitl` (symbol resolution and load/call guardrails)
- BETA-103 -> `src/vitte/stdlib/reflection/reflection.vitl` (metadata/introspection path docs alignment)
- BETA-104 -> `src/vitte/stdlib/profiling/profiler.vitl` (hotspot/stat computation consistency)
- BETA-105 -> `src/vitte/stdlib/packages/package.vitl` (registry connection visibility)

## Feature request policy
- Implement top 3 requests only if no Critical/Major remains.
