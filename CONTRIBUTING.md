# Contributing to Vitte

Thank you for your interest in contributing to Vitte! This document outlines how to contribute effectively.

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md). We're committed to providing a welcoming and inspiring community for all.

## How to Contribute

### Reporting Bugs

1. **Check existing issues** - Avoid duplicates by searching [GitHub Issues](https://github.com/vitte-lang/vitte/issues)
2. **Create a detailed report** including:
   - OS and Vitte version
   - Steps to reproduce
   - Expected behavior
   - Actual behavior
   - Error messages or logs

### Suggesting Enhancements

1. **Describe the feature** - Explain what problem it solves or what improvement it provides
2. **Provide examples** - Show how you'd like to use it
3. **Consider alternatives** - Discuss other approaches or related features

### Submitting Pull Requests

1. **Fork the repository** on GitHub
2. **Create a feature branch**:
   ```bash
   git checkout -b feature/my-feature
   ```
3. **Make your changes**:
   - Follow the [Code Style](#code-style) guidelines
   - Write clear commit messages
   - Add tests for new functionality
4. **Commit and push**:
   ```bash
   git commit -m "Add feature: description"
   git push origin feature/my-feature
   ```
5. **Open a Pull Request** with:
   - Clear description of changes
   - Reference to related issues
   - Test results showing your changes work

## Development Setup

### Prerequisites

- Git
- Rust (for building the Rust components)
- C17 compiler (gcc, clang, or MSVC)
- Make or CMake

### Quick Start

```bash
# Clone the repository
git clone https://github.com/vitte-lang/vitte.git
cd vitte

# Run bootstrap (one-time setup)
./scripts/bootstrap_stage0.sh
./scripts/self_host_stage1.sh

# Build the compiler
make build

# Run tests
make test
```

For detailed setup, see [INSTALL.md](INSTALL.md).

## Code Style

### C Code

- **Indentation**: 4 spaces (no tabs)
- **Line length**: Aim for 80 characters, max 120
- **Naming**:
  - Functions: `snake_case`
  - Types: `PascalCase`
  - Constants: `SCREAMING_SNAKE_CASE`
  - Private functions: prefix with `_`
- **Comments**: Use `//` for single lines, `/* */` for blocks
- **Braces**: Allman style (opening brace on same line)

### Rust Code

Follow standard Rust conventions (enforced by `rustfmt`):

```bash
# Format Rust code
cargo fmt --manifest-path rust/Cargo.toml

# Run clippy linter
cargo clippy --manifest-path rust/Cargo.toml
```

### Commit Messages

Use clear, concise messages:

```
feat: Add type inference for generics

- Implement constraint generation
- Add unification algorithm
- Update type checker tests

Closes #123
```

**Prefixes**:
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation
- `test:` - Test additions/improvements
- `refactor:` - Code refactoring
- `perf:` - Performance improvement
- `chore:` - Maintenance tasks

## Testing

### Running Tests

```bash
# Run all tests
make test

# Run specific test suite
make test-compiler
make test-runtime
make test-stdlib

# Run with coverage
make test-coverage
```

### Writing Tests

For compiler changes, add tests in `compiler/tests/`:

```c
// Test new feature
void test_new_feature(void) {
    const char* source = "let x = 42";
    // Test compilation and behavior
}
```

For stdlib changes, add tests in `std/tests/`:

```vitte
#[test]
fn test_new_function() {
    assert_eq!(new_function(5), expected_value)
}
```

## Documentation

### Adding Documentation

- **Code comments**: Explain *why*, not *what*
- **Function/module docs**: Use doc comments
- **User-facing**: Add to `docs/`
- **API reference**: Update [docs/api/](docs/api/)

### Documentation Format

```c
/// Adds two integers
///
/// # Arguments
/// - a: first integer
/// - b: second integer
///
/// # Returns
/// The sum of a and b
int add(int a, int b) {
    return a + b;
}
```

## Community

- **Forum**: Discuss ideas on GitHub Discussions
- **Chat**: Join our [Discord server](https://discord.gg/vitte)
- **Meetings**: Weekly sync on Thursdays at 5 PM UTC

## Review Process

1. **Automated checks** - CI must pass (tests, linting, formatting)
2. **Code review** - At least one maintainer reviews
3. **Approval** - Requires approval from maintainers
4. **Merge** - Squash commits into main branch

## License

By contributing, you agree that your contributions will be licensed under the [License](LICENSE) of this project.

## Questions?

- Check our [FAQ](docs/troubleshooting.md)
- Open an issue on GitHub
- Reach out to the maintainers

Thank you for contributing to Vitte! 🎉
