# Security Policy

## Reporting Security Vulnerabilities

**Please do not open public GitHub issues for security vulnerabilities.**

Instead, email security@vitte-lang.dev with:

1. **Vulnerability description** - What is the issue?
2. **Affected versions** - Which versions are impacted?
3. **Reproduction steps** - How can we reproduce it?
4. **Impact** - What are the consequences?
5. **Suggested fix** (optional) - Do you have ideas for a patch?

### Response Timeline

- **48 hours**: Initial acknowledgment
- **1 week**: Initial assessment and response plan
- **30 days**: Target patch release (may vary by severity)
- **Public disclosure**: After patch is available

## Security Considerations

### Memory Safety

Vitte is designed with memory safety in mind:

- **Ownership system** - Prevents use-after-free bugs
- **Borrow checker** - Ensures data races are impossible
- **No null pointers** - Uses `Option<T>` for nullable values

However, `unsafe` blocks can bypass these guarantees. Use unsafe code only when necessary and review carefully.

### Type Safety

Vitte's type system provides compile-time guarantees:

- **Type checking** - Catches type errors before runtime
- **Pattern matching** - Forces handling of all cases
- **Error types** - No hidden null/nil failures

### Dependency Security

- Regular security audits of dependencies
- Timely updates for critical security patches

## Security Best Practices for Users

When using Vitte:

1. **Keep updated** - Use the latest stable version
2. **Minimize unsafe code** - Review all `unsafe` blocks carefully
3. **Validate input** - Don't trust untrusted data
4. **Use strong types** - Let the type system help you
5. **Handle errors** - Use `Result<T, E>` for fallible operations
6. **Audit dependencies** - Review what you depend on

## Supported Versions

Security updates are provided for:

| Version | Status | Until |
|---------|--------|-------|
| 1.x | LTS (Long-term support) | TBD |
| 0.2 | Current | Next major release |
| 0.1 | End of life | No security updates |

## Security Advisories

Published advisories can be found in [GitHub Security Advisories](https://github.com/vitte-lang/vitte/security/advisories).

## Third-party Dependencies

Vitte uses the following security-critical dependencies:

- See [LICENSE-THIRD-PARTY.md](LICENSE-THIRD-PARTY.md) for full license and security info

## Questions?

For security questions, email security@vitte-lang.dev.

---

We take security seriously. Thank you for helping keep Vitte safe.
