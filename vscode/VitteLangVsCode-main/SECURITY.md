# Security Policy (Suggestions Engine)

## Defaults
- Cloud backend is disabled by default.
- Local-only mode is enabled by default.
- Secret redaction is enabled by default.
- Trusted workspace check is enabled by default.

## Cloud Usage Requirements
Cloud requests are only allowed when all conditions are met:
1. `vitte.suggestions.aiPipeline.enabled = true`
2. `vitte.suggestions.aiPipeline.backendEnabled = true`
3. `vitte.suggestions.aiPipeline.cloudOptIn = true`
4. `vitte.suggestions.aiPipeline.localOnly = false`
5. If `privacyStrict = true`, backend host must match `backendAllowlist`
6. If `trustedWorkspaceOnly = true`, workspace must be trusted

## Secret Redaction
When `redactSecrets = true`, outgoing context attempts to redact:
- API keys/tokens (`sk-`, `ghp_`, JWT-like, Slack-like)
- password/secret/token assignments
- PEM private key blocks
- `.env`-like lines

## Incident Response
If suspicious leakage is suspected:
1. Switch to local-only mode
2. Disable backend and revoke tokens
3. Rotate affected credentials
4. Re-open with cloud only after review
