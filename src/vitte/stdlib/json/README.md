# json

Path: `src/vitte/stdlib/json`

## Purpose

Structured JSON surfaces including parse, parser, types, builder, schema, stringify, and serialize.

## Architecture Role

Use `json` when data must cross a structured textual boundary. This family owns JSON shape and conversion, not business validation itself.

## Main Responsibilities

- Own JSON parsing and rendering concerns.
- Keep structured exchange separate from file access and domain acceptance.
- Document the distinction between raw JSON shape and validated program state.

## Module Inventory

- `builder.vitl`
- `parse.vitl`
- `parser.vitl`
- `schema.vitl`
- `serialize.vitl`
- `stringify.vitl`
- `types.vitl`

## Complete Integration Story

- A build plan can be validated as domain data first and then exported to JSON in a separate step.
- A parser page should say how syntax becomes JSON values before serialization details appear.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
