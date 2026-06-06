# encoding

Path: `src/vitte/stdlib/encoding`

## Purpose

Text and byte encoding surfaces such as utf, base64, url, hex, html, legacy encodings, and unicode helpers.

## Architecture Role

Use `encoding` when values cross a textual or byte-oriented boundary and representation matters.

## Main Responsibilities

- Own representation conversions instead of business meaning.
- Keep wire/text transformations separate from structured domain logic.
- Explain which submodule is responsible for which external format.

## Module Inventory

- `base64.vitl`
- `hex.vitl`
- `html.vitl`
- `legacy.vitl`
- `unicode.vitl`
- `url.vitl`
- `utf.vitl`
- `utf8.vitl`

## Complete Integration Story

- A JSON payload can be rendered first, then base64-encoded for transport.
- A path or URL can be normalized before joining it with host-facing code.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
