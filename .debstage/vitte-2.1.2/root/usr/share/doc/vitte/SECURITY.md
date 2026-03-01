# Security Policy

Thanks for reporting vulnerabilities responsibly.

Vitte is experimental, but security issues are treated seriously.
This document explains scope, reporting path, and expected response.

## Supported Versions

Security triage priority:

1. `main` branch
2. latest published release

Older revisions may be reviewed best-effort only.

## How to Report

### Preferred

- Open a **private** security report (if platform settings allow).

### Fallback

- Open a public issue with minimal details and request a private channel.
- Do not include exploit payloads or sensitive data in public.

## What to Include

Please include:

- vulnerability summary
- impact (RCE, privilege escalation, data leak, DoS, etc.)
- exact reproduction steps
- minimal PoC
- affected commit/version
- environment (OS, compiler/toolchain, flags)
- any known workaround

A concise report gets fixed faster.

## Scope

In scope:

- compiler (`src/compiler/**`)
- language/runtime behavior shipped in this repo
- package modules under `src/vitte/packages/**`
- official tooling in `tools/` and maintained editor integrations

Out of scope:

- vulnerabilities only present in third-party dependencies
- local environment misconfiguration
- unofficial forks with unrelated changes

## Response Targets

Target timelines (best effort):

- acknowledgement: within 72h
- first triage: within 7 days
- remediation plan: as soon as severity is confirmed

Severe issues are prioritized immediately.

## Severity Guidance

Rough priority model:

- `Critical`: remote code execution, full compromise
- `High`: privilege escalation, major memory corruption, major data exposure
- `Medium`: controlled DoS, limited data leaks, sandbox escape preconditions
- `Low`: hard-to-exploit edge cases with low practical impact

Final severity is assigned by maintainers after triage.

## Disclosure Rules

- no public full disclosure before fix availability
- no exploitation against production systems
- no data exfiltration beyond minimal proof
- coordinate publication timing with maintainers

After a fix is available and users had time to update, coordinated public disclosure is welcome.

## Credit

No bug bounty is offered currently.
Contributors can be credited in release notes on request.

## Encryption

If encrypted communication is required, request key exchange in the initial contact.

## Related Docs

- `SUPPORT.md` for non-security issues
- `CONTRIBUTING.md` for normal contribution workflow
