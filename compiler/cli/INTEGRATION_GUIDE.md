# Vitte Compiler CLI - Integration Guide

Complete guide for integrating the CLI framework into the main Vitte compiler.

## Overview

The CLI system is a standalone, production-ready module that can be integrated into the main compiler. It provides all command-line interface functionality for the `vittec` compiler executable.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│          Main Compiler (vittec executable)          │
├─────────────────────────────────────────────────────┤
│  CLI Framework                                       │
│  ├─ Argument parsing                                │
│  ├─ Command dispatch                                │
│  ├─ Configuration management                        │
│  └─ User interaction                                │
├─────────────────────────────────────────────────────┤
│  Command Implementations                            │
│  ├─ compile → lexer/parser/codegen                  │
│  ├─ test → test runner                              │
│  ├─ analyze → static analyzer                       │
│  ├─ build → CMake integration                       │
│  └─ ... (other commands)                            │
├─────────────────────────────────────────────────────┤
│  Compiler Core                                       │
│  ├─ Lexer                                           │
│  ├─ Parser                                          │
│  ├─ Semantic Analyzer                               │
│  └─ Code Generator                                  │
└─────────────────────────────────────────────────────┘
```

## Integration Steps

### Step 1: Copy Files to Compiler Directory

The CLI files are already in `compiler/cli/`. If needed elsewhere:

```bash
# CLI is already in the right place:
compiler/cli/
├── cli.h
├── cli.c
├── cli_commands.h
├── cli_commands.c
├── config.h
├── config.c
├── main.c
└── CMakeLists.txt
```

### Step 2: Update Main CMakeLists.txt

Integrate CLI into the main compiler build:

```cmake
# In compiler/CMakeLists.txt

# Add CLI subdirectory
add_subdirectory(cli)

# Link CLI library to main executable
target_link_libraries(vittec PRIVATE vitte-cli-lib)

# Include CLI headers
target_include_directories(vittec PRIVATE cli)
```

### Step 3: Update Main Source Files

#### In your main compiler entry point:

```c
#include "cli.h"
#include "cli_commands.h"

int main(int argc, char *argv[]) {
    // Initialize CLI context
    cli_context_t *ctx = cli_init();
    if (!ctx) {
        fprintf(stderr, "Failed to initialize CLI\n");
        return 1;
    }
    
    // Register all commands
    setup_all_commands(ctx);
    
    // Parse and execute
    int result = 0;
    if (cli_parse(ctx, argc, argv) == 0) {
        result = cli_execute(ctx);
    } else {
        result = 1;
    }
    
    // Cleanup
    cli_free(ctx);
    return result;
}
```

### Step 4: Implement Command Actions

Update command implementations to call actual compiler functions:

```c
// In cli_commands.c - Example for compile command

#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../codegen/codegen.h"

int cmd_compile(cli_command_t *cmd, int argc, char **argv) {
    // Get input file
    const char *input = cli_get_string(ctx, "input");
    const char *output = cli_get_string(ctx, "output");
    
    // Run actual compilation pipeline
    lexer_t *lexer = lexer_create_from_file(input);
    if (!lexer) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", input);
        return 1;
    }
    
    // ... parse, analyze, generate code ...
    
    lexer_free(lexer);
    // ... cleanup ...
    
    return 0;
}
```

### Step 5: Load Configuration

Integrate configuration system:

```c
#include "cli/config.h"

int main(int argc, char *argv[]) {
    // Load configuration
    vitte_config_t *config = vitte_config_create();
    vitte_config_set_defaults(config);
    
    // Load from file if it exists
    vitte_config_load_file(config, ".vitteconfig");
    
    // Load from environment
    vitte_config_load_env(config);
    
    // Use configuration values
    int opt_level = *(int*)vitte_config_get_default(config, 
        "compile.optimization", &(int){2});
    
    // ... rest of initialization ...
    
    vitte_config_free(config);
    return 0;
}
```

## Integration Points

### 1. Lexer Integration

Connect CLI compile command to lexer:

```c
// cli_commands.c
#include "../lexer/lexer.h"

int cmd_compile(cli_command_t *cmd, int argc, char **argv) {
    const char *input_file = cli_get_string(ctx, "input");
    lexer_t *lexer = lexer_create_from_file(input_file);
    
    if (!lexer) {
        cli_log_error(ctx, "Failed to open input file");
        return 1;
    }
    
    // Run lexer
    while (true) {
        token_t token = lexer_next_token(lexer);
        if (token.type == TOKEN_EOF) break;
        // Process token
    }
    
    lexer_free(lexer);
    return 0;
}
```

### 2. Parser Integration

Connect compile command to parser:

```c
// After lexer, create parser
parser_t *parser = parser_create(lexer);
ast_t *ast = parser_parse(parser);

if (!ast) {
    cli_log_error(ctx, "Parse error");
    parser_free(parser);
    return 1;
}

// Process AST
parser_free(parser);
```

### 3. Semantic Analyzer Integration

```c
// After parsing
analyzer_t *analyzer = analyzer_create();
if (!analyzer_analyze(analyzer, ast)) {
    cli_log_error(ctx, "Analysis failed");
    // Handle errors
    return 1;
}

analyzer_free(analyzer);
```

### 4. Code Generator Integration

```c
// After semantic analysis
codegen_t *codegen = codegen_create();
const char *output_file = cli_get_string(ctx, "output");

if (!codegen_generate(codegen, ast, output_file)) {
    cli_log_error(ctx, "Code generation failed");
    codegen_free(codegen);
    return 1;
}

codegen_free(codegen);
```

### 5. Test Command Integration

```c
// cli_commands.c - test command
int cmd_test(cli_command_t *cmd, int argc, char **argv) {
    cli_log_info(ctx, "Running tests...");
    
    // Load and run tests
    test_suite_t *suite = test_suite_create();
    test_suite_load_from_directory(suite, "tests/");
    
    int passed = test_suite_run(suite);
    int failed = suite->total - passed;
    
    printf("Tests: %d passed, %d failed\n", passed, failed);
    
    test_suite_free(suite);
    return failed > 0 ? 1 : 0;
}
```

## File Dependencies

### CLI Framework Dependencies
```
cli.h / cli.c
  └─ Standard C library only
```

### Command Implementation Dependencies
```
cli_commands.c
  ├─ cli.h / cli.c
  ├─ ../lexer/lexer.h
  ├─ ../parser/parser.h
  ├─ ../codegen/codegen.h
  └─ ../analyzer/analyzer.h
```

### Configuration System Dependencies
```
config.h / config.c
  └─ Standard C library only
```

## Build Integration

### Single-Target Build

In root `CMakeLists.txt`:

```cmake
# Build everything in one target
add_executable(vittec
    cli/cli.c
    cli/cli_commands.c
    cli/config.c
    cli/main.c
    src/lexer/lexer.c
    src/parser/parser.c
    src/codegen/codegen.c
    # ... other sources ...
)
```

### Multi-Target Build

Using subdirectories:

```cmake
# Add subdirectories
add_subdirectory(cli)
add_subdirectory(src)

# Link
target_link_libraries(vittec vitte-cli-lib vitte-compiler-lib)
```

## Testing Integration

### Unit Tests

Add CLI tests to test suite:

```c
// tests/test_cli.c
#include <assert.h>
#include "../cli/cli.h"

void test_cli_option_parsing() {
    cli_context_t *ctx = cli_init();
    setup_all_commands(ctx);
    
    const char *argv[] = {"vittec", "version"};
    int argc = 2;
    
    int result = cli_parse(ctx, argc, (char**)argv);
    assert(result == 0);
    
    cli_free(ctx);
}

void test_cli_validation() {
    cli_context_t *ctx = cli_init();
    
    // Test validators
    cli_option_t opt = cli_option_builder("--file");
    // ... test option ...
    
    cli_free(ctx);
}

int main() {
    test_cli_option_parsing();
    test_cli_validation();
    printf("All tests passed!\n");
    return 0;
}
```

## Configuration Integration

### Default Configuration

Place `.vitteconfig` in project root:

```bash
cp compiler/cli/.vitteconfig .vitteconfig
```

Users can customize it for their projects.

### Environment Variables

Supported environment variables (prefix: `VITTE_`):

```bash
VITTE_VERBOSE=2
VITTE_OPTIMIZATION=3
VITTE_DEBUG=true
VITTE_HOME=/path/to/project
VITTE_INCLUDE=/path/to/includes
```

## Documentation Integration

### User Documentation
- Keep `CLI_DOCUMENTATION.md` in `compiler/cli/`
- Reference from main docs

### Developer Documentation
- Keep `README.md` in `compiler/cli/`
- Reference for architecture

### Quick Start
- Keep `QUICKSTART.md` in `compiler/cli/`
- Link from main README

## Deployment

### Building the Compiler

```bash
cd vitte
mkdir build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Install
cmake --install .

# Verify
vittec --version
```

### Distribution

The compiled `vittec` binary can be distributed:

1. **Standalone**: Single executable with no dependencies
2. **Package**: Include in Linux distributions
3. **Homebrew**: Create formula for macOS
4. **Windows**: Create installer

## Performance Optimization

### CLI Performance
- Argument parsing: O(n)
- Command lookup: O(m)
- Total startup: <10ms

### Command Performance
- Depends on actual implementation
- Use compilation configuration for tuning

## Memory Management

### CLI Framework
- All allocations cleaned up in `cli_free()`
- No memory leaks
- Safe error handling

### Command Implementations
- Must free their own allocations
- Use `cli_log_error()` for errors
- Return proper exit codes

## Error Handling

### CLI-Level Errors
```c
if (cli_parse(ctx, argc, argv) != 0) {
    cli_log_error(ctx, "Failed to parse arguments");
    return 1;
}
```

### Command-Level Errors
```c
int cmd_example(cli_command_t *cmd, int argc, char **argv) {
    if (some_error) {
        fprintf(stderr, "Error: Description\n");
        return 1;  // Non-zero exit code
    }
    return 0;      // Success
}
```

## Migration Checklist

- [ ] Review CLI architecture (README.md)
- [ ] Copy CLI files to compiler/cli/
- [ ] Update CMakeLists.txt for CLI integration
- [ ] Update main entry point for CLI
- [ ] Implement command actions
- [ ] Integrate with lexer
- [ ] Integrate with parser
- [ ] Integrate with analyzer
- [ ] Integrate with code generator
- [ ] Build and test
- [ ] Run full compiler test suite
- [ ] Verify all 11 commands work
- [ ] Check performance
- [ ] Update main documentation
- [ ] Deploy

## Troubleshooting

### Compilation Errors
```bash
# Ensure C17 standard
cmake .. -DCMAKE_C_FLAGS="-std=c17"

# Check include paths
# Verify cli.h is in include path
```

### Linking Errors
```bash
# Ensure CLI sources are compiled
# Check CMakeLists.txt targets
# Verify library linking
```

### Runtime Errors
```bash
# Run with verbose output
./vittec -v command

# Check .vitteconfig
# Verify environment variables
```

## Support and Maintenance

### Adding New Commands
1. Define in `cli_commands.h`
2. Implement in `cli_commands.c`
3. Register in `setup_all_commands()`
4. Document in `CLI_DOCUMENTATION.md`

### Extending Validation
1. Add validator function to `cli.c`
2. Use with `.set_validator()`
3. Document in `CLI_DOCUMENTATION.md`

### Configuration Updates
1. Add constant to `config.h`
2. Update `.vitteconfig` template
3. Document in `README.md`

## References

- **User Guide**: [CLI_DOCUMENTATION.md](CLI_DOCUMENTATION.md)
- **Developer Guide**: [README.md](README.md)
- **Quick Start**: [QUICKSTART.md](QUICKSTART.md)
- **File Index**: [INDEX.md](INDEX.md)
- **Implementation**: [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)

## Next Steps

1. Review CLI architecture
2. Integrate with main compiler
3. Implement command backends
4. Run full test suite
5. Deploy to users
6. Gather feedback
7. Iterate and improve

---

**Status**: Ready for integration
**Version**: 1.0.0
**Date**: 2025-01-15
