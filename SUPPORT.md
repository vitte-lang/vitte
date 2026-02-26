# Support

Need help with Vitte? This page explains where to ask and how to get useful answers quickly.

## Where to Ask

For general help:

- open a GitHub issue in this repository
- include a clear title and a minimal reproducible example

For security reports:

- do **not** post sensitive details publicly
- follow `SECURITY.md`

## What to Include

Please include as much of this as possible:

- what you tried
- what you expected
- what happened instead
- exact command used
- full error output
- commit hash (`git rev-parse --short HEAD`)
- OS + compiler/toolchain versions

Good issue template:

```text
Context:
Expected:
Actual:
Command:
Error output:
Environment:
```

## Fast Local Checks (Before Asking)

Run:

```sh
make build
make parse
make hir-validate
```

If your issue is module-related, also run:

```sh
make modules-tests
make modules-snapshots
```

## Response Expectations

Vitte is an experimental project, so response time can vary.

- maintainers try to triage quickly
- small, reproducible reports are handled first
- unclear reports may be closed until more details are provided

## Scope

Best support coverage:

- compiler behavior
- language syntax/diagnostics
- official tooling in `tools/`
- package/module workflows under `src/vitte/packages`

Limited coverage:

- custom forks with large local divergence
- third-party dependency internals
- unrelated system configuration issues

## Practical Tips

- keep reports focused on one problem
- attach the smallest `.vit` file that reproduces the issue
- prefer exact logs over screenshots

Thanks for helping improve Vitte.
