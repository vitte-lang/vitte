# Vitte Compiler CLI - Complete Documentation

## Overview

The Vitte Compiler CLI (`vittec`) is an enterprise-grade command-line interface for the Vitte programming language compiler. It provides comprehensive tools for compilation, testing, analysis, and project management.

## Quick Start

```bash
# Compile a Vitte program
vittec compile main.vitte

# Run tests
vittec test --verbose

# Build a project
vittec build --release

# Show help
vittec help
```

## Commands

### 1. compile - Compile Source Files

Compiles Vitte source code to C or native assembly.

**Usage:**
```bash
vittec compile [options] <input_file>
```

**Options:**
- `-o, --output <file>` - Output file path
- `-t, --target <target>` - Target platform (x86-64, arm64, wasm)
- `-O, --optimize <level>` - Optimization level (0-3)
- `--emit-ir` - Emit intermediate representation
- `--emit-asm` - Emit assembly code
- `--emit-c` - Emit C code
- `-g, --debug` - Include debug symbols
- `-W` - Treat warnings as errors
- `-v, --verbose` - Verbose output

**Examples:**
```bash
# Basic compilation
vittec compile main.vitte

# With optimization
vittec compile -O3 main.vitte

# Emit intermediate representation
vittec compile --emit-ir main.vitte

# Debug build with verbose output
vittec compile -g -v main.vitte
```

### 2. test - Run Test Suite

Executes unit and integration tests.

**Usage:**
```bash
vittec test [options] [pattern]
```

**Options:**
- `--unit` - Run unit tests only
- `--integration` - Run integration tests only
- `--coverage` - Generate coverage report
- `--timeout <seconds>` - Test timeout
- `-v, --verbose` - Verbose output

**Examples:**
```bash
# Run all tests
vittec test

# Run with coverage
vittec test --coverage

# Run specific test pattern
vittec test --unit "test_lexer"

# Verbose test output
vittec test --verbose
```

### 3. analyze - Code Analysis

Performs static analysis on source code.

**Usage:**
```bash
vittec analyze [options] <input_file>
```

**Options:**
- `--metrics` - Show code metrics
- `--dependencies` - Show dependency graph
- `--symbols` - Show symbol table
- `-v, --verbose` - Verbose output

**Examples:**
```bash
# Full analysis
vittec analyze main.vitte

# Show metrics only
vittec analyze --metrics main.vitte

# Show dependencies
vittec analyze --dependencies main.vitte
```

### 4. format - Code Formatting

Reformats code to match style guidelines.

**Usage:**
```bash
vittec format [options] <input_file>
```

**Options:**
- `-i, --in-place` - Modify file in place
- `--check` - Check formatting without modifying
- `--style <style>` - Code style (vitte, google, llvm)

**Examples:**
```bash
# Check formatting
vittec format --check main.vitte

# Format in place
vittec format -i main.vitte

# With specific style
vittec format --style google -i main.vitte
```

### 5. lint - Static Analysis

Analyzes code for common issues and best practices.

**Usage:**
```bash
vittec lint [options] <input_file>
```

**Options:**
- `--warnings` - Show warnings
- `--errors` - Show errors
- `--strict` - Enable strict checking
- `-v, --verbose` - Verbose output

**Examples:**
```bash
# Run linter
vittec lint main.vitte

# Strict mode
vittec lint --strict main.vitte

# With verbose output
vittec lint -v main.vitte
```

### 6. build - Build Project

Builds the entire project using CMake.

**Usage:**
```bash
vittec build [options] [target]
```

**Options:**
- `--debug` - Debug build
- `--release` - Release build
- `-j, --jobs <n>` - Number of parallel jobs
- `--clean` - Clean before building

**Examples:**
```bash
# Debug build
vittec build --debug

# Release build with 4 jobs
vittec build --release -j4

# Clean and rebuild
vittec build --release --clean
```

### 7. install - Install Compiler

Installs the compiler to the system.

**Usage:**
```bash
vittec install [options]
```

**Options:**
- `--prefix <path>` - Installation prefix
- `--user` - Install for current user only
- `--system` - System-wide installation

**Examples:**
```bash
# Install for current user
vittec install --user

# System-wide installation
vittec install --system

# Custom prefix
vittec install --prefix /opt/vitte
```

### 8. clean - Clean Build Artifacts

Removes temporary and build files.

**Usage:**
```bash
vittec clean [options]
```

**Options:**
- `--all` - Remove all build artifacts
- `--cache` - Remove cache files only

**Examples:**
```bash
# Clean all artifacts
vittec clean --all

# Clean cache only
vittec clean --cache
```

### 9. doc - Generate Documentation

Generates API documentation from source code.

**Usage:**
```bash
vittec doc [options] [output_dir]
```

**Options:**
- `--format <format>` - Output format (html, pdf, markdown)
- `--theme <theme>` - Documentation theme
- `-v, --verbose` - Verbose output

**Examples:**
```bash
# Generate HTML documentation
vittec doc --format html docs/

# Generate PDF
vittec doc --format pdf docs/

# With custom theme
vittec doc --format html --theme dark docs/
```

### 10. version - Show Version Information

Displays compiler version and build information.

**Usage:**
```bash
vittec version
```

**Example Output:**
```
Vitte Compiler v1.0.0
Build: 2025-01-15
Copyright: Vitte Project
License: MIT
```

### 11. help - Show Help Information

Displays comprehensive help information.

**Usage:**
```bash
vittec help [command]
```

**Examples:**
```bash
# Show general help
vittec help

# Show help for compile command
vittec help compile

# Show help for test command
vittec help test
```

## Global Options

These options work with any command:

- `-h, --help` - Show help message
- `-v, --version` - Show version
- `-V, --verbose` - Verbose output
- `-q, --quiet` - Quiet mode
- `--color` - Force colored output
- `--no-color` - Disable colored output

## Configuration

### Configuration File

Create a `.vitteconfig` file in your project root:

```ini
# Output settings
output.format=text
output.color=true

# Compilation settings
compile.optimization=2
compile.emit_ir=false
compile.emit_asm=false
compile.debug=false

# Build settings
build.parallel=true
build.threads=4

# Cache settings
cache.directory=.vitte/cache
```

### Environment Variables

Set environment variables to override configuration:

```bash
# Compilation verbosity
export VITTE_VERBOSE=2

# Optimization level
export VITTE_OPTIMIZATION=3

# Debug mode
export VITTE_DEBUG=true

# Project root
export VITTE_HOME=/path/to/project
```

## Exit Codes

- `0` - Success
- `1` - General error
- `2` - Command not found
- `3` - Invalid arguments
- `4` - File not found
- `5` - Permission denied
- `6` - Compilation error
- `7` - Runtime error

## Examples

### Basic Compilation Workflow

```bash
# Check formatting
vittec format --check main.vitte

# Run linter
vittec lint main.vitte

# Compile in debug mode
vittec compile -g main.vitte

# Run tests
vittec test --coverage

# Build release version
vittec build --release

# Generate documentation
vittec doc --format html docs/
```

### Project Build Pipeline

```bash
# Clean previous build
vittec clean --all

# Build debug version
vittec build --debug

# Run tests with coverage
vittec test --coverage

# Analyze code quality
vittec analyze --metrics .

# Generate documentation
vittec doc --format html docs/

# Build release version
vittec build --release

# Install binaries
vittec install --user
```

### CI/CD Integration

```bash
# In continuous integration
vittec lint --strict .
vittec compile -W main.vitte
vittec test --coverage
vittec analyze --metrics .
```

## Performance Tips

1. **Parallel Builds**: Use `-j` flag for parallel compilation
   ```bash
   vittec build --release -j$(nproc)
   ```

2. **Optimization Levels**:
   - `-O0` - No optimization (default in debug)
   - `-O1` - Basic optimization
   - `-O2` - Moderate optimization (default in release)
   - `-O3` - Aggressive optimization

3. **Incremental Compilation**: Use build cache
   ```bash
   vittec compile --cache main.vitte
   ```

4. **Distributed Building**: Use ccache or sccache
   ```bash
   CCACHE_DIR=/tmp/vitte-cache vittec build --release
   ```

## Troubleshooting

### Compilation Fails

```bash
# Get verbose output
vittec compile -v main.vitte

# Check for lint errors
vittec lint main.vitte

# Check dependencies
vittec analyze --dependencies main.vitte
```

### Tests Fail

```bash
# Run specific test with verbose output
vittec test --verbose "failing_test"

# Check compiler version
vittec version

# Clean and rebuild
vittec clean --all
vittec build --debug
```

### Build Issues

```bash
# Clean build
vittec build --clean --debug

# Check configuration
vittec config --list

# Verbose build output
vittec build -v --debug
```

## Architecture

The CLI is built on a modular architecture:

- **cli.h/cli.c** - Core CLI framework with option parsing
- **cli_commands.h/cli_commands.c** - Command implementations
- **config.h/config.c** - Configuration management
- **main.c** - Entry point

## Contributing

To add a new command:

1. Add command function to `cli_commands.c`:
   ```c
   int cmd_mycommand(cli_context_t *ctx, cli_command_t *cmd, int argc, char **argv) {
       // Implementation
       return 0;
   }
   ```

2. Add setup function:
   ```c
   cli_command_t cli_setup_mycommand_command(void) {
       cli_command_t cmd = {
           .name = "mycommand",
           .description = "Description",
           .execute = cmd_mycommand,
       };
       return cmd;
   }
   ```

3. Register in `setup_all_commands()`:
   ```c
   cli_register_command(ctx, cli_setup_mycommand_command());
   ```

Global options such as `--profile` and `--config` are parsed before command dispatch and available via the `cli_context_t` passed into each executor. Help output automatically groups commands by their `.group` key, so set it to place the command under the right section.

## License

This implementation is part of the Vitte Compiler project and follows the same license terms.

## Support

For issues or questions:
- Visit: https://github.com/vitte-lang/vitte
- Documentation: https://vitte-lang.org/docs
- Community: https://community.vitte-lang.org
