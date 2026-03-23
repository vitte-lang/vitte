# Repository Hygiene

This document keeps repository cleanliness explicit instead of leaving it to taste.

For a compiler and language project, repository order is part of credibility.

## Goals

- keep the repository root legible
- avoid committing machine-specific artifacts
- make documentation and tooling easier to navigate
- reduce low-signal churn in reviews

## Root Rules

The repository root should contain only:

- top-level project docs
- build/config files
- clearly named top-level source or support directories

Avoid leaving ambiguous one-off files in the root unless they are intentionally part of the public project workflow.

## Generated Artifact Rules

Do not commit:

- local OS metadata files
- editor scratch files
- local build outputs
- temporary diagnostics logs
- machine-specific cache files

If a generated file is intentionally committed, it should be documented as:

- generated
- source of truth or derived artifact
- how it is refreshed

## Documentation Rules

- add new top-level docs to `docs/README.md`
- prefer one clear source-of-truth document per topic
- keep high-level docs short and link outward instead of duplicating content

## Tooling Rules

- keep script names descriptive
- prefer grouping scripts by domain as the tool inventory grows
- avoid adding one-off scripts without a clear owner or purpose

## Review Rule

If a PR adds a new root file or generated artifact, the PR should explain why that file belongs at that level of the repository.
