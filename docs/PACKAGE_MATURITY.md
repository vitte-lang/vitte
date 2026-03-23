# Package Maturity

This document classifies package families by maturity expectation.

It does not claim every documented package is fully stable.
It gives users and contributors a clearer default reading.

## Maturity Levels

| Level | Meaning |
| --- | --- |
| `stable` | covered by an explicit compatibility promise or protected contract |
| `documented-experimental` | publicly documented, but still expected to change |
| `internal` | not a public package promise |

## Current Working Classification

### `stable`

The strongest current stability claim is still the language core itself, not the whole package inventory.

Package families should only be treated as `stable` when they are explicitly covered by a compatibility policy.

At the moment, users should assume no broad package-family stability beyond what explicit docs guarantee.

### `documented-experimental`

These package families are visible public surface because they have dedicated docs or API indexes, but they should still be read as experimental by default unless promoted:

- `std`
- `fs`
- `db`
- `http`
- `http_client`
- `process`
- `log`
- `json`
- `yaml`
- `lint`
- `test`

### `internal`

The following should be read as internal by default:

- `src/vitte/packages/**/internal/*`
- package implementation details without dedicated public docs
- package families without a documented public-facing contract

## Promotion Rules

A package family should be promoted from `documented-experimental` toward `stable` only when all of these are true:

1. it has a clear README or API entrypoint
2. ownership and package-shape requirements are enforced
3. contract or compatibility checks exist
4. examples or user-facing workflows exist
5. release notes can describe its public changes clearly

If any of these are missing, promotion is premature.

## Demotion Rule

A package family may need to move back to `documented-experimental` if:

- compatibility expectations were overstated
- gates are not keeping up with churn
- the documented contract is wider than the tested contract

## Reader Rule

When evaluating a package family:

1. check whether it has dedicated docs
2. check whether it has explicit compatibility language
3. assume `documented-experimental` unless the docs say more
