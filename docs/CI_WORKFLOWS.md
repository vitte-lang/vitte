# CI Workflows

This page explains the main GitHub workflows and why they exist.

## Workflow Map

### `.github/workflows/all-tests.yml`

Main regression workflow.

Purpose:

- build the project
- run grouped regression suites
- isolate module/package-heavy checks
- run completions validation separately
- upload normalized reports

Use this as the main reference when someone asks "what does CI really run?"

### `.github/workflows/grammar-check.yml`

Focused grammar workflow.

Purpose:

- validate grammar-related changes quickly
- keep grammar-generated artifacts in sync
- avoid waiting for the full repository matrix on grammar-only changes

### `.github/workflows/debian-crash-regressions.yml`

Focused runtime and packaging regression workflow.

Purpose:

- catch Debian/install-path regressions
- exercise crash-report snapshot flows
- keep packaging-related compiler readiness visible

### `.github/workflows/modules-report-pr.yml`

PR-facing modules reporting workflow.

Purpose:

- surface module/package reporting information in review-friendly form

### `.github/workflows/modules-snapshots-pr.yml`

PR-facing module snapshot workflow.

Purpose:

- validate package/module snapshot expectations during review

### `.github/workflows/modules-weekly-v3.yml`

Periodic modules workflow.

Purpose:

- run recurring module/package-oriented checks that are useful as ongoing maintenance signals

## Practical Reading Rule

If a PR changes:

- grammar: start with `grammar-check`
- broad compiler behavior: expect `all-tests`
- packages/modules: expect the module workflows and module gates to matter
- packaging or runtime readiness: expect Debian-focused checks to matter
