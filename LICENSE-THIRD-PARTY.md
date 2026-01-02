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

### Development Dependencies

No additional language-specific package managers are required beyond the C toolchain (CMake/Ninja/Clang/GCC).

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

We maintain dependencies by regularly reviewing upstream releases and running standard security/advisory checks for third-party libraries.
- Prefer minimal dependency sets.
- Review changelogs and licenses before upgrading.

## Adding New Dependencies

When adding new dependencies:

1. **Check license compatibility** - Ensure it's compatible with Apache 2.0 / MIT
2. **Verify security** - Check for known advisories or CVEs
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
