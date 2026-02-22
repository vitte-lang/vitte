# VitteOS Scripts Interface Contract

This contract keeps VitteOS majority logic in Vitte while allowing external tooling scripts.

## Ownership

- Vitte orchestrates workflow and source-of-truth behavior.
- Shell/Python/Node scripts are tooling helpers only.

## Allowed Script Scope

- Build/bootstrap wrappers.
- Lint/check automation.
- Reporting/matrix generation.
- Environment diagnostics.

## Disallowed Script Scope

- Implementing core kernel behavior.
- Replacing Vitte module logic.
- Encoding architecture state transitions that belong in `.vit`.

## Entry Points

- Main orchestrator: `vitteos/scripts/vitteos_tooling.vit`
- Tool wrappers:
  - `vitteos/tooling/bootstrap.sh`
  - `vitteos/tooling/check_vit.py`
  - `vitteos/tooling/run_check.js`

## CI Enforcement

- `make vitteos-scripts-check`
- `make vitteos-scripts-check-soft`
- `make vitteos-issues-check`
- `make vitteos-domain-contract`
- `make vitteos-no-orphan-check`
- `make vitteos-space-naming-lint`
- `make vitteos-vit-targeted-check`
- `make vitteos-vit-header-lint`
- `make vitteos-kernel-smoke`
- `make vitteos-adr-policy-check`
