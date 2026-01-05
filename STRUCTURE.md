# Vitte Repository Structure

This repository is organized into three main components reflecting Vitte's architecture:

## vittec/ - Vitte Compiler
The compiler implementation organized into 5 logical domains:
- **frontend/**: Lexing, parsing, and AST transformations  
- **middle/**: Type checking, analysis, and HIR processing
- **backend/**: Code generation and MIR optimization
- **infrastructure/**: Core utilities and data structures
- **support/**: Error handling, diagnostics, and configuration

See [vittec/ARCHITECTURE.md](vittec/ARCHITECTURE.md) for detailed documentation.

## stdlib/ - Standard Library  
Core and standard library implementations organized into 5 categories:
- **core/**: Core library (no allocation required)
- **alloc/**: Heap-based data structures
- **std/**: Full standard library with OS support
- **runtime/**: Runtime support and panic handling
- **extra/**: Optional and platform-specific components

See [stdlib/ORGANIZATION.md](stdlib/ORGANIZATION.md) for detailed documentation.

## src/ - Source Tools and Documentation
Supporting tools, documentation, and build infrastructure:
- **bootstrap/**: Bootstrap compiler configuration
- **build_helper/**: Build system helpers
- **ci/**: Continuous integration scripts
- **config/**: Configuration files
- **doc/**: Vitte documentation and guides
- **etc/**: Miscellaneous support files
- **external/**: External dependencies (gcc, llvm-project)
- **runtime/**: Runtime tools (vittedoc, vitte-std-workspace)
- **tools/**: Development tools

See [src/README.md](src/README.md) for detailed information on source organization.

## Design Philosophy

This structure follows Vitte's design principle of clarity and organization:
- **Clear separation of concerns**: Each domain/category has a specific purpose
- **Layered architecture**: Lower layers are independent of upper layers
- **Maintainability**: Related functionality is grouped together
- **Extensibility**: New features can be added within appropriate domains
