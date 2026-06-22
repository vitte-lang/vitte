# math

Path: `src/vitte/stdlib/math`

## Purpose

Arithmetic, algebra, comparison, calculus, geometry, modular arithmetic, number theory, probability, statistics, matrix, and vector helpers.

## Architecture Role

Use `math` when the transformation itself is the feature. This family exists so algorithmic intent stays visible and testable.

## Main Responsibilities

- Own numeric and symbolic transformations beyond one-off helpers.
- Group mathematical domains into explicit submodules.
- Let readers see what kind of computation a program is performing.

## Module Inventory

- `algebra.vitl`
- `arithmetic.vitl`
- `arrays.vitl`
- `calculus.vitl`
- `comparison.vitl`
- `complex.vitl`
- `geometry.vitl`
- `logic.vitl`
- `matrix.vitl`
- `modular.vitl`
- `number_theory.vitl`
- `powers.vitl`
- `probability.vitl`
- `roots.vitl`
- `sequences.vitl`
- `sort.vitl`
- `statistics.vitl`
- `topology.vitl`
- `trigonometry.vitl`
- `vector.vitl`

## Complete Integration Story

- A scoring engine can compute aggregates in `math` while keeping I/O and transport elsewhere.
- A statistics or matrix chapter should explain the workflow around the computation, not just a single formula.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
