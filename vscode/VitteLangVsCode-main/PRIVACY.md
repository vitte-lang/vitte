# Privacy Policy (Suggestions Engine)

## Local-Only First
The extension is configured to run local-only by default:
- `cloudOptIn = false`
- `localOnly = true`

No remote suggestion context is sent unless user explicitly opts in.

## Data Sent to Backend (Opt-In Only)
When cloud is enabled and consented, each request sends:
- up to 320 characters immediately before the cursor
- up to five import names from the active document
- selected local source snippets, each capped at 700 characters and limited by `ragTopK`
- candidate count and declared policy metadata (`dataRetentionDays`, `allowExternalTraining`)

Requests omit absolute file paths, source file names, diagnostics, build/test configuration, dependency lists and Git history. Secret-like text is redacted when `redactSecrets` is enabled. The configured endpoint receives source text only after the explicit cloud opt-in; when disabled or offline, suggestions stay local and no request is made.

## Data Retention
`dataRetentionDays` expresses desired retention policy to backend.
Recommended default: `0` (no retention requested).

## External Training / Snippet Compliance
`allowExternalTraining` defaults to `false`.
This should stay false unless legal/licensing compliance is validated.

## User Controls
- `Vitte: Suggestions Cloud Opt-In`
- `Vitte: Suggestions Cloud Opt-Out (Local-Only)`
- `Vitte: Suggestions Refresh Context`

## Recommendation
For sensitive codebases:
- keep local-only mode
- keep privacy strict mode
- keep secret redaction enabled
- use backend allowlist with explicit domains only
