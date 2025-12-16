# CLI Application Template

## Overview

This template provides a complete CLI application structure for Vitte.

## Files

- `muffin.muf` - Package manifest
- `src/main.vit` - Entry point
- `src/cli.vit` - CLI argument parsing
- `src/commands/` - Command implementations
- `tests/` - Test files

## Building

```bash
vitte-build
./my-cli --help
```

## Project Structure

```
my-cli/
├── muffin.muf
├── src/
│   ├── main.vit
│   ├── cli.vit
│   └── commands/
│       ├── mod.vit
│       ├── run.vit
│       └── build.vit
├── tests/
│   └── integration_test.vit
└── examples/
    └── basic.vit
```

## Key Dependencies

- `std` - Standard library
- `cli` - Command-line parsing (optional)
- `anyhow` - Error handling (optional)
