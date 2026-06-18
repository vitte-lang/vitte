# Privacy Policy (Suggestions Engine)

## Local-Only First
The extension is configured to run local-only by default:
- `cloudOptIn = false`
- `localOnly = true`

No remote suggestion context is sent unless user explicitly opts in.

## Data Sent to Backend (Opt-In Only)
When cloud is enabled and consented, request payload can include:
- cursor-left context
- related repository chunks (RAG top-k)
- import hints and active diagnostics
- declared policy metadata (`dataRetentionDays`, `allowExternalTraining`)

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
