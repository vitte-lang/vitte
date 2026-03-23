# Known Limitations

Vitte is experimental overall.

This page exists so that limitations are explicit instead of being discovered indirectly.

## Current Limitations

- broad language stability is not promised beyond the protected core
- documented package families are visible public surface, but many still default to `documented-experimental`
- native Windows support is experimental
- performance reporting is still baseline-oriented rather than fully regression-gated
- repository breadth is ahead of a fully unified maturity story across every package and adjacent surface
- backend and generated-artifact details should not be read as broad compatibility promises unless docs say so

## Practical Reading Rule

If a surface is:

- documented and gated: treat it as higher-confidence
- documented but not explicitly promoted: treat it as experimental by default
- internal or undocumented: do not treat it as public promise

## Maintenance Rule

If a recurring limitation disappears, remove it here.
If a new important caveat appears, add it here in the same PR that introduced it.
