

# Vitte Performance Regressions

Generated: 2026-06-21 15:00 UTC

---

## Summary

| Metric | Value |
|----------|----------|
| Total Examples | 407 |
| Stable Examples | 396 |
| Regressions Detected | 11 |
| Critical Regressions | 3 |
| Warning Regressions | 5 |
| Average Delta | 14.2% |
| Worst Delta | 78% |

---

## Top Regressions

| Example | Old | New | Delta | Category | Severity |
|----------|----------|----------|----------|----------|----------|
| compiler/typeck.vit | 51ms | 91ms | +78% | compiler | critical |
| async/future.vit | 14ms | 25ms | +78% | async | critical |
| runtime/task.vit | 31ms | 50ms | +61% | runtime | critical |

---

## Category Summary

| Category | Examples | Regressions | Worst Delta |
|----------|----------|----------|----------|
| compiler | 120 | 5 | 78% |
| async | 42 | 2 | 78% |
| runtime | 65 | 3 | 61% |
| stdlib | 180 | 1 | 12% |

---

## Regression Policy

| Delta | Classification |
|----------|----------|
| < 5% | Stable |
| 5% - 20% | Watch |
| 20% - 50% | Warning |
| > 50% | Critical |

---

## Compiler Pipeline

```text
Lexer
  ↓
Parser
  ↓
AST
  ↓
HIR
  ↓
Sema
  ↓
Typeck
  ↓
Borrowck
  ↓
MIR
  ↓
IR
  ↓
Backend
  ↓
Linker
```

---

## History

| Field | Value |
|----------|----------|
| Baseline | 20260620T220000Z.json |
| Current | 20260621T150000Z.json |

---

## Generated Artifacts

```text
build/examples/examples-matrix.json
build/examples/examples-matrix.md
build/examples/examples-matrix.html
build/examples/examples-matrix.xml
build/examples/regressions.json
build/examples/regressions.md
build/examples/regression.html
build/examples/history/
```
