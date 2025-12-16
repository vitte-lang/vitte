# Changelog

All notable changes to the Vitte programming language and toolchain are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Documentation structure (`docs/` directory)
- Comprehensive tutorial series (Getting Started through Concurrency)
- Code examples for common patterns
- Contributing guidelines
- Community governance documents

### Changed
- Project structure reorganization for better documentation

### Fixed
- [Pending items from development]

## [0.2.0] - December 2024

### Added
- Initial language specification (`vitte.ebnf`)
- Multi-stage bootstrap compiler (`vittec`)
- Muffin package manifest format
- Standard library core modules
- Platform Abstraction Layer (PAL) design
- Runtime ABI specification
- Comprehensive test infrastructure
- Fuzzing infrastructure with multiple targets
- FFI support for C interoperability
- Compiler diagnostics system

### Changed
- Restructured project for production readiness

### Known Issues
- Runtime implementation in progress
- Some standard library modules not yet implemented
- Error messages need refinement

## [0.1.0] - Initial Release

### Added
- Language design and specification
- Compiler skeleton
- Basic runtime structure
- SDK scaffolding
- Standard library structure

---

## How to Track Changes

When contributing changes:

1. Add entries to the `[Unreleased]` section
2. Use categories: Added, Changed, Deprecated, Removed, Fixed, Security
3. Link to related issues: `([#123](https://github.com/vitte-lang/vitte/issues/123))`
4. When releasing, move Unreleased to new version with date

Example entry:
```markdown
### Fixed
- Fixed type inference for generic functions ([#456](https://github.com/vitte-lang/vitte/issues/456))
```

---

## Release Process

1. Update version in `VERSION` and relevant `mod.muf` files
2. Update this CHANGELOG
3. Create git tag: `git tag v0.3.0`
4. Build and test release artifacts
5. Publish announcement
6. Create GitHub release

See [MAINTAINERS.md](MAINTAINERS.md) for detailed procedures.
