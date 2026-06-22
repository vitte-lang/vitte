# io

Path: `src/vitte/stdlib/io`

## Purpose

File, buffer, stream, stdio, and host-runtime access helpers.

## Architecture Role

Use `io` when the program must read or write bytes, files, or streams. Keep it separate from validation, parsing, or business decisions.

## Main Responsibilities

- Own file and stream movement.
- Clarify buffered vs direct access paths.
- Keep host interaction visible in the architecture.

## Module Inventory

- `buffer.vitl`
- `file.vitl`
- `fileops.vitl`
- `host_runtime.vitl`
- `io.vitl`
- `stdio.vitl`
- `stream.vitl`

## Complete Integration Story

- A manifest is loaded through `io`, parsed elsewhere, validated elsewhere, and only then emitted back through `io`.
- A stdio helper should explain where user-facing text enters the flow.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
