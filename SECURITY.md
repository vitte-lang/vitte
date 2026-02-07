# Security Policy

Thanks for reporting vulnerabilities responsibly. This document describes scope, contact, and expectations.

## Supported Versions

Vitte is experimental. We prioritize issues affecting the `main` branch and the latest published release.

## Responsible Disclosure

Please avoid public disclosure before a fix is available. A useful report includes:

- Clear description of the issue
- Steps to reproduce
- Minimal PoC
- Estimated impact (RCE, data leak, DoS, etc.)
- Affected version/commit
- Environment details (OS, toolchain, flags)

## Contact

- Open a private issue if possible
- Otherwise open a public issue **without sensitive details**, and request a private channel

## Response Timeline

- Acknowledgement: within 72h
- Initial triage: within 7 days
- Fix or plan: as soon as possible based on severity

## Scope

In scope:
- Vitte compiler
- Vitte runtime
- Standard library
- Official tooling (scripts in `tools/`, editor integrations)

Out of scope:
- Third‑party dependencies (OpenSSL, libcurl, etc.)
- Local modifications or unofficial forks

## Safe Disclosure Rules

- Do not exploit in production
- Do not exfiltrate real data
- Keep PoC minimal

## Rewards

No bug bounty at the moment. Security contributors can be credited in release notes on request.

## Encryption

If you need encrypted contact, propose a public key and we will respond with ours.

---

## FAQ

**Can I publish after a fix ships?**  
Yes, after coordinated disclosure.

**What if I’m unsure it’s a vulnerability?**  
Send it anyway; we’ll triage.
