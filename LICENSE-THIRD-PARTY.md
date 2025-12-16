# Third-Party Licenses

This document lists third-party dependencies and their licenses.

## Vitte Dependencies

### Direct Dependencies

#### C/C++ Projects

| Name | Version | License | Purpose | Notes |
|------|---------|---------|---------|-------|
| CMake | 3.20+ | BSD | Build system | |
| Meson | 0.63+ | Apache 2.0 | Build system | Alternative to CMake |
| Unity Test Framework | Latest | MIT | Unit testing | For C tests |

#### Rust Crates

Use `cargo tree --licenses` to see full Rust dependency tree.

Core dependencies:
- `serde` - MIT/Apache-2.0 - Serialization
- `parking_lot` - MIT/Apache-2.0 - Synchronization primitives
- `log` - MIT/Apache-2.0 - Logging
- `thiserror` - MIT/Apache-2.0 - Error handling

### Development Dependencies

| Name | License | Purpose |
|------|---------|---------|
| Cargo | MIT/Apache-2.0 | Rust package manager |
| Rustfmt | MIT/Apache-2.0 | Code formatting |
| Clippy | MIT/Apache-2.0 | Linting |
| Criterion | Apache-2.0 | Benchmarking |

## License Compliance

### License Types Used

- **Apache 2.0** - Permissive, requires notice
- **MIT** - Permissive, minimal restrictions
- **BSD-2-Clause** - Permissive, similar to MIT
- **MPL 2.0** - Weak copyleft

### Terms Summary

**Permissive Licenses (MIT, Apache 2.0, BSD)**:
- ✓ Use commercially
- ✓ Modify source code
- ✓ Distribute
- ✓ Private use
- ⚠ Include license text
- ⚠ Include copyright notice

**Copyleft Licenses (if any)**:
- Reciprocal licensing obligations apply
- See specific license for details

## Dependency Updates

We maintain dependencies using:

- **Cargo audit** - Check for known vulnerabilities
- **Dependabot** - Automated dependency updates
- **Regular updates** - Keep dependencies current

Run locally:
```bash
cargo audit
cargo update
```

## Adding New Dependencies

When adding new dependencies:

1. **Check license compatibility** - Ensure it's compatible with Apache 2.0 / MIT
2. **Verify security** - Run `cargo audit`
3. **Minimize dependencies** - Prefer small, focused crates
4. **Document** - Update this file

## License of Vitte

Vitte itself is licensed under:

- **Compiler (vittec)**: Apache License 2.0
- **Standard Library (std)**: Apache License 2.0
- **SDK**: Apache License 2.0
- **Documentation**: Creative Commons Attribution 4.0

See [LICENSE](LICENSE) for full text.

## Questions about Licenses?

For license-related questions, email legal@vitte-lang.dev.

---

Last updated: December 2024
