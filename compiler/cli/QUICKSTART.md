# Vitte Compiler CLI - Quick Start Guide

Get the Vitte compiler CLI up and running in 5 minutes.

## Prerequisites

- CMake 3.16+
- C compiler (GCC, Clang, or MSVC)
- Standard C library

## Installation

### 1. Clone the Repository
```bash
git clone https://github.com/vitte-lang/vitte.git
cd vitte/compiler/cli
```

### 2. Build
```bash
# Create build directory
mkdir build
cd build

# Configure (Release mode)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .

# Install (optional)
cmake --install .
```

### 3. Verify Installation
```bash
# Show version
./vittec version

# Show help
./vittec help
```

## First Commands

### Compile a File
```bash
./vittec compile main.vitte
```

### Run Tests
```bash
./vittec test --verbose
```

### Build a Project
```bash
./vittec build --release
```

### Format Code
```bash
./vittec format -i main.vitte
```

### Show Help
```bash
./vittec help compile
```

## Configuration

### Create Project Configuration
```bash
# Copy default configuration
cp .vitteconfig ~/my_project/.vitteconfig

# Edit for your project
nano ~/my_project/.vitteconfig
```

### Set Environment Variables
```bash
# Verbosity
export VITTE_VERBOSE=2

# Optimization
export VITTE_OPTIMIZATION=3

# Debug mode
export VITTE_DEBUG=true
```

## Common Workflows

### Development Build
```bash
# Compile with debug info
vittec compile -g main.vitte

# Run tests
vittec test --coverage

# Check code quality
vittec lint main.vitte
```

### Release Build
```bash
# Format code
vittec format -i main.vitte

# Run linter
vittec lint --strict main.vitte

# Compile with optimization
vittec compile -O3 main.vitte

# Build release
vittec build --release

# Generate docs
vittec doc --format html docs/
```

### CI/CD Pipeline
```bash
# Lint (strict)
vittec lint --strict .

# Compile with warnings as errors
vittec compile -W main.vitte

# Run tests with coverage
vittec test --coverage

# Analyze code
vittec analyze --metrics .

# All succeeded - ready to deploy
```

## Troubleshooting

### Command Not Found
```bash
# Make sure vittec is in PATH
export PATH="./build:$PATH"

# Or use full path
./build/vittec --version
```

### Compilation Fails
```bash
# Get verbose output
./vittec compile -v main.vitte

# Check for lint errors
./vittec lint main.vitte
```

### Build Issues
```bash
# Clean and rebuild
./vittec clean --all
./vittec build --debug
```

## Next Steps

1. **Read Full Documentation**
   - [CLI_DOCUMENTATION.md](CLI_DOCUMENTATION.md) - Complete user guide
   - [README.md](README.md) - Architecture and developer guide

2. **Explore Examples**
   - [cli_examples.c](cli_examples.c) - 8 working examples
   - Shows all major features

3. **Understand Configuration**
   - [.vitteconfig](.vitteconfig) - Default settings
   - Customize for your needs

4. **Integration**
   - Link with your compiler
   - Use as library or executable
   - See [README.md](README.md) for integration guide

## Quick Command Reference

| Command | Purpose |
|---------|---------|
| `compile` | Compile Vitte source files |
| `test` | Run test suite |
| `analyze` | Analyze code structure |
| `format` | Format code |
| `lint` | Check code quality |
| `build` | Build project |
| `install` | Install compiler |
| `clean` | Clean artifacts |
| `doc` | Generate documentation |
| `version` | Show version info |
| `help` | Show help |

## Quick Options Reference

| Option | Purpose |
|--------|---------|
| `-h, --help` | Show help |
| `-v, --version` | Show version |
| `-V, --verbose` | Verbose output |
| `-q, --quiet` | Quiet mode |
| `-O<level>` | Optimization (0-3) |
| `-g, --debug` | Debug symbols |
| `-o, --output` | Output file |
| `-W` | Warnings as errors |
| `--release` | Release mode |
| `--debug` | Debug mode |

## Directory Structure

```
compiler/cli/
├── cli.h               - Core API
├── cli.c               - Implementation
├── cli_commands.h      - Commands header
├── cli_commands.c      - Commands impl
├── config.h            - Config API
├── config.c            - Config impl
├── main.c              - Entry point
├── CMakeLists.txt      - Build config
├── cli_examples.c      - Usage examples
├── .vitteconfig        - Config template
├── README.md           - Developer guide
├── CLI_DOCUMENTATION.md - User guide
├── INDEX.md            - File navigation
└── IMPLEMENTATION_SUMMARY.md
```

## Getting Help

### In Terminal
```bash
vittec help
vittec help compile
vittec compile --help
```

### In Documentation
- Quick Start: This file
- Full User Guide: [CLI_DOCUMENTATION.md](CLI_DOCUMENTATION.md)
- Architecture: [README.md](README.md)
- File Index: [INDEX.md](INDEX.md)

### Code Examples
- Working Examples: [cli_examples.c](cli_examples.c)
- Configuration: [.vitteconfig](.vitteconfig)

## Building for Different Platforms

### Linux (GCC/Clang)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### macOS
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
cmake --build .
```

### Windows (MSVC)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022"
cmake --build . --config Release
```

## Performance Tips

1. **Parallel Build**: Use multiple cores
   ```bash
   vittec build --release -j$(nproc)
   ```

2. **Optimization Levels**
   - `-O0` - No optimization (default debug)
   - `-O1` - Basic
   - `-O2` - Moderate (default release)
   - `-O3` - Aggressive

3. **Caching**: Use build cache
   ```bash
   vittec compile --cache main.vitte
   ```

4. **Incremental**: Only rebuild changed files
   - Automatic with most build systems

## Version Information

- **Version**: 1.0.0
- **Build Date**: 2025-01-15
- **License**: MIT
- **Status**: Production Ready

## License

This CLI is part of the Vitte Compiler project, licensed under MIT License.

---

**Ready to start?**
```bash
./build/vittec compile main.vitte
```

**Questions?** See [CLI_DOCUMENTATION.md](CLI_DOCUMENTATION.md)
