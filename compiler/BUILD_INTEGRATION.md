# Integration Guide

## Files Created

### Complete List of New Files

**Headers (18 files):**
1. `include/compiler/ast.h` - Abstract Syntax Tree
2. `include/compiler/backend.h` - Backend abstraction
3. `include/compiler/backend_c.h` - C code emission
4. `include/compiler/codegen.h` - Code generation
5. `include/compiler/compiler.h` - Main compiler context
6. `include/compiler/diagnostic.h` - Error diagnostics
7. `include/compiler/driver.h` - Compilation driver
8. `include/compiler/frontend.h` - Frontend abstraction
9. `include/compiler/hir.h` - High-level IR
10. `include/compiler/ir.h` - Low-level IR
11. `include/compiler/lexer.h` - Tokenizer
12. `include/compiler/lowering.h` - AST to IR lowering
13. `include/compiler/optimizer.h` - IR optimizations
14. `include/compiler/parser.h` - Parser
15. `include/compiler/sema.h` - Semantic analysis
16. `include/compiler/symbol_table.h` - Symbol tracking
17. `include/compiler/target.h` - Target architecture
18. `include/compiler/types.h` - Type system

**Master Header:**
- `include/compiler.h` - Single include for all modules

**Implementations (18 files):**
- `src/ast.c`
- `src/backend.c`
- `src/backend_c.c`
- `src/codegen.c`
- `src/compiler.c`
- `src/diagnostic.c`
- `src/driver.c`
- `src/frontend.c`
- `src/hir.c`
- `src/ir.c`
- `src/lexer.c`
- `src/lowering.c`
- `src/optimizer.c`
- `src/parser.c`
- `src/sema.c`
- `src/symbol_table.c`
- `src/target.c`
- `src/types.c`

**Build Configuration:**
- `build/CMakeLists.txt` - Build output configuration
- `build/README.md` - Build instructions
- `build/.gitignore` - Ignore build artifacts
- `CMakeLists_full.txt` - Full compiler build configuration

**Documentation:**
- `COMPILER_FILES.md` - Summary of all files
- `BUILD_INTEGRATION.md` - This file

## Integration Steps

### 1. Update Top-Level CMakeLists.txt

Add compiler module to root CMakeLists.txt:

```cmake
# In root CMakeLists.txt
add_subdirectory(compiler)
```

### 2. Link with Bootstrap Compiler

The compiler now has:
- **Bootstrap compiler** (existing) - `src/front/`, `src/back/`, `src/diag/`, `src/support/`
- **New compiler framework** - All the headers and implementations above

These should coexist and eventually the bootstrap compiler can be gradually replaced.

### 3. Build Instructions

**Option A: Using CMakeLists_full.txt**
```bash
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../
make
```

**Option B: Using existing build system**
If your project has its own build system, add the new `.c` files to compilation.

### 4. File Dependencies

**No External Dependencies:**
- All new files use only C standard library (stdlib.h, string.h, etc.)
- No external libraries required
- Compatible with existing bootstrap compiler

**Header Include Path:**
```c
#include "compiler/ast.h"
#include "compiler/parser.h"
// ... etc
```

Or use the master header:
```c
#include "compiler.h"  // Includes all modules
```

## Compilation Structure

```
vittec executable
├── main.c              (entry point)
├── vittec.c            (CLI interface)
├── version.c
├── frontend pipeline
│   ├── lexer.c         (tokenization)
│   ├── parser.c        (parsing)
│   └── sema.c          (semantic analysis)
├── IR pipeline
│   ├── ast.c           (AST manipulation)
│   ├── hir.c           (high-level IR)
│   ├── ir.c            (low-level IR)
│   └── lowering.c      (AST -> IR)
├── backend pipeline
│   ├── optimizer.c     (IR optimizations)
│   ├── codegen.c       (code generation)
│   └── backend_c.c     (C emission)
└── support modules
    ├── types.c         (type system)
    ├── symbol_table.c  (name resolution)
    ├── diagnostic.c    (error reporting)
    ├── target.c        (architecture info)
    └── driver.c        (main driver)
```

## Adding to Build

### CMake Integration

```cmake
# src/CMakeLists.txt or similar
target_sources(vittec PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer.c
    # ... add all .c files
)

target_include_directories(vittec PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
```

### Makefile Integration

```makefile
COMPILER_SOURCES = \
    compiler/src/ast.c \
    compiler/src/parser.c \
    compiler/src/lexer.c \
    # ... add all .c files

VITTEC_SOURCES += $(COMPILER_SOURCES)
```

## Next Steps

1. **Implement lexer.c** - Replace TODO with full tokenization
2. **Implement parser.c** - Build complete parser
3. **Implement types.c** - Complete type system
4. **Implement sema.c** - Semantic analysis and type checking
5. **Implement lowering.c** - AST to IR transformation
6. **Test each module** - Write unit tests for each component
7. **Integrate optimizer.c** - IR optimization passes
8. **Complete backend_c.c** - C code emission

## Checking Integration

Verify files are in place:

```bash
# Check headers
ls compiler/include/compiler/*.h  # Should show 18 files

# Check sources
ls compiler/src/*.c | wc -l      # Should show 18+ files

# Check build files
ls compiler/build/                # Should show CMakeLists.txt, README.md, .gitignore
```

All 39+ files have been created successfully!
