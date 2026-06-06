# data

Path: `src/vitte/stdlib/data`

## Purpose

Dataset-oriented helpers such as schema, transform, merge, cleaning, and statistics.

## Architecture Role

Use `data` when the program manipulates rows, records, schemas, or staged transformations instead of one-off scalar logic.

## Main Responsibilities

- Own data-shaping operations that are richer than basic containers.
- Keep schema and transform steps explicit in the docs.
- Clarify how datasets evolve through a pipeline.

## Module Inventory

- `cleaning.vitl`
- `data.vitl`
- `dataset.vitl`
- `merge.vitl`
- `schema.vitl`
- `stats.vitl`
- `transform.vitl`

## Complete Integration Story

- A telemetry report can be cleaned, merged, transformed, and summarized before export.
- A schema page should explain what a valid record looks like before code samples start.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
