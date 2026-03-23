# Release Policy

Vitte does not claim broad language stability yet.

This policy exists to make project publication clearer without pretending the project is more stable than it is.

## Release Channels

Vitte uses three practical channels:

| Channel | Meaning | Intended audience |
| --- | --- | --- |
| `main` | active development branch | contributors and early adopters following daily changes |
| `rc` | release candidate for a bounded validation window | maintainers and testers validating a publishable state |
| `snapshot` | published point-in-time build or source state | external users who want a named reference point |

## What Each Channel Means

### `main`

`main` moves quickly.

- breaking changes are allowed
- docs should still remain honest and current
- core gates should stay green
- package and tooling work may still evolve significantly

### `rc`

An `rc` should exist only when maintainers want a short validation window before publishing a snapshot.

Minimum criteria:

- `make build` passes
- `make core-language-gate` passes
- `make core-release-gate` passes
- documented release notes and known limitations are current
- public docs do not overclaim stability

### `snapshot`

A snapshot is the recommended lightweight publication unit while the project remains experimental.

A snapshot should be associated with:

- a commit or tag
- a short note on current protected language guarantees
- current known limitations
- current high-signal platform expectations

## What A Snapshot Does Not Mean

A snapshot does not mean:

- broad semantic versioning guarantees
- full stdlib stability
- compatibility across all experimental features
- long-term support

## Suggested Snapshot Checklist

Before publishing a snapshot:

1. verify `README.md` and `docs/PUBLIC_SURFACE.md` still match reality
2. run the documented core gates
3. check the current project health summary
4. refresh known limitations if needed
5. ensure the release notes do not imply more stability than the gates protect

## Release Note Rule

Release notes should distinguish clearly between:

- protected core behavior
- experimental additions
- internal refactors with no public impact

If a change is not inside the documented public perimeter, it should not be written like a public guarantee.
