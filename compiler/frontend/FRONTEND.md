# Vitte Compiler - Frontend Documentation

Complete frontend system for the Vitte compiler with lexer, parser, and semantic analyzer.

## Overview

The frontend converts Vitte source code into an Abstract Syntax Tree (AST) with full type checking and semantic validation.

```
Source Code
    ↓
Lexer (lexer.h/c)      - Tokenization
    ↓
Tokens
    ↓
Parser (parser.h/c)    - Syntax analysis
    ↓
AST
    ↓
Semantic Analyzer      - Type checking
(semantic.h/c)
    ↓
Typed AST → Ready for code generation
```

## Components

### 1. Lexer (lexer.h / lexer.c - 800+ lines)

Converts source code into tokens.

**Features:**
- 50+ token types
- Keyword recognition
- Number parsing (integers, floats, hex, octal, binary)
- String and character literal handling
- Comment skipping (single-line and multi-line)
- Operator and delimiter tokenization
- Position tracking (line, column)
- Error reporting

**Key Functions:**
```c
lexer_t* lexer_create(const char *source);
lexer_t* lexer_create_from_file(const char *filename);
int lexer_tokenize(lexer_t *lexer);
token_t lexer_next_token(lexer_t *lexer);
```

**Token Types:**
- Literals: INTEGER, FLOAT, STRING, CHAR, BOOL, NIL
- Keywords: fn, let, var, if, else, for, while, return, etc.
- Operators: +, -, *, /, ==, !=, <, >, &&, ||, etc.
- Delimiters: (), {}, [], ;, :, ,

### 2. Parser (parser.h / parser.c - 800+ lines)

Converts tokens into an Abstract Syntax Tree.

**Features:**
- Recursive descent parsing
- Operator precedence handling
- Error recovery
- Expression parsing
- Statement parsing
- Function definitions
- Control flow statements
- Type annotations (ready)

**AST Node Types:**
- Program, Function, Variable
- Assignment, Binary/Unary operations
- Call expressions, Indexing, Member access
- If/For/While/Match statements
- Return/Break/Continue statements
- Blocks, Arrays, Type definitions

**Key Functions:**
```c
parser_t* parser_create(token_t *tokens, size_t token_count);
ast_node_t* parser_parse(parser_t *parser);
ast_node_t* parser_parse_expression(parser_t *parser);
ast_node_t* parser_parse_statement(parser_t *parser);
```

### 3. Semantic Analyzer (semantic.h / semantic.c - 600+ lines)

Type checks and validates the AST.

**Features:**
- Type system (25+ types)
- Symbol table management
- Scoping (global, local, function)
- Type checking and inference
- Error reporting
- Scope management

**Type System:**
```
Primitive: void, bool, int, uint, float, string, char
Sized: i8, i16, i32, i64, u8, u16, u32, u64, f32, f64
Complex: pointer, array, function
User-defined: struct, enum, trait
Generic: generic, unknown
```

**Key Functions:**
```c
semantic_analyzer_t* analyzer_create(void);
int analyzer_analyze(semantic_analyzer_t *analyzer, ast_node_t *ast);
void analyzer_define_symbol(...);
symbol_t* analyzer_lookup_symbol(...);
type_info_t* analyzer_get_type(...);
```

## Usage Example

### Basic Compilation Pipeline

```c
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

int main(void) {
    // Read source code
    const char *source = "fn main() { }";
    
    // Lexical analysis
    lexer_t *lexer = lexer_create(source);
    lexer_tokenize(lexer);
    
    // Syntax analysis
    parser_t *parser = parser_create(lexer->tokens, lexer->token_count);
    ast_node_t *ast = parser_parse(parser);
    
    // Semantic analysis
    semantic_analyzer_t *analyzer = analyzer_create();
    analyzer_analyze(analyzer, ast);
    
    // Check for errors
    size_t error_count;
    const char **errors = analyzer_get_errors(analyzer, &error_count);
    if (error_count > 0) {
        for (size_t i = 0; i < error_count; i++) {
            printf("Error: %s\n", errors[i]);
        }
    }
    
    // Cleanup
    ast_node_free(ast);
    parser_free(parser);
    analyzer_free(analyzer);
    lexer_free(lexer);
    
    return 0;
}
```

## File Structure

```
frontend/
├── lexer.h                - Lexer API (50+ tokens)
├── lexer.c                - Lexer implementation (800+ lines)
├── parser.h               - Parser API
├── parser.c               - Parser implementation (800+ lines)
├── semantic.h             - Semantic analyzer API
├── semantic.c             - Analyzer implementation (600+ lines)
├── test_frontend.c        - Unit tests (400+ lines)
├── CMakeLists.txt         - Build configuration
└── FRONTEND.md            - This documentation
```

## Building

### With CMake

```bash
cd compiler/frontend
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Running Tests

```bash
./test-frontend
```

### Expected Output

```
=== Vitte Compiler - Frontend Test Suite ===

--- LEXER TESTS ---
✓ PASS - Basic Tokenization
✓ PASS - Number Lexing
✓ PASS - String Lexing
✓ PASS - Keyword Recognition
✓ PASS - Operator Lexing

--- PARSER TESTS ---
✓ PASS - Basic Parsing
✓ PASS - Function Parsing
✓ PASS - If Statement Parsing
✓ PASS - For Loop Parsing

--- SEMANTIC ANALYSIS TESTS ---
✓ PASS - Type Creation
✓ PASS - Type Compatibility
✓ PASS - Semantic Analysis
✓ PASS - Symbol Table

Test Results: 13/13 passed
```

## Supported Language Features

### Literals
- Integers: `42`, `0xFF`, `0o77`, `0b1010`
- Floats: `3.14`, `1e-5`, `1.0e+2`
- Strings: `"hello"`
- Characters: `'a'`
- Booleans: `true`, `false`
- Nil: `nil`

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`, `**`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `&&`, `||`, `!`
- Bitwise: `&`, `|`, `^`, `~`, `<<`, `>>`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`
- Other: `.`, `->`, `=>`, `..`, `...`, `::`

### Statements
- Variable declaration: `let x = 42;`
- Variable mutation: `var x = 42;`
- Function definition: `fn add(a: int, b: int) -> int { }`
- If statement: `if (cond) { } else { }`
- For loop: `for (init; cond; update) { }`
- While loop: `while (cond) { }`
- Return: `return 42;`
- Break/Continue: `break;`, `continue;`

### Type Annotations
- Primitive types: `int`, `float`, `string`, `bool`
- Sized integers: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`
- Pointers: `int*`
- Arrays: `[10]int`
- Functions: `fn(int, int) -> int`

## Performance

- **Tokenization**: O(n) where n = source length
- **Parsing**: O(n) for most code, O(n²) worst case with deep nesting
- **Type Checking**: O(m) where m = AST nodes
- **Memory**: Proportional to AST size

## Statistics

| Metric | Value |
|--------|-------|
| Source Lines | 2,200+ |
| Token Types | 50+ |
| AST Node Types | 20+ |
| Type System | 25+ types |
| Built-in Keywords | 32 |
| Error Recovery | Yes |
| Position Tracking | Yes |

## Error Handling

All three components provide detailed error messages:

```c
// Lexer errors
- "Unterminated string"
- "Invalid number format"

// Parser errors
- "Expected ')' after arguments"
- "Unexpected token"
- "Expected '{' for block"

// Semantic errors
- "Undefined identifier"
- "Type mismatch"
- "Return outside function"
- "Break/Continue outside loop"
```

## Integration with Code Generator

The frontend outputs a fully-typed AST ready for code generation:

```
AST with types → IRGen → LLVM IR → Object code
                ↓
            Assembly
```

## Future Enhancements

1. **Generics**: Full generic type support
2. **Pattern Matching**: Advanced match expressions
3. **Traits**: Trait bounds and implementations
4. **Lifetime Analysis**: Reference lifetime tracking
5. **Module System**: Module resolution and imports
6. **Better Error Messages**: With code snippets
7. **Type Inference**: More sophisticated inference
8. **Const Evaluation**: Compile-time evaluation

## Testing

Comprehensive test suite with 13+ test cases:
- Lexer: Basic tokens, numbers, strings, keywords, operators
- Parser: Expressions, statements, functions, control flow
- Semantic: Types, compatibility, symbol table, scoping

Run tests with:
```bash
./test-frontend
```

## Architecture Highlights

1. **Separation of Concerns**: Each phase (lex, parse, analyze) is independent
2. **Error Recovery**: Parser continues after errors for better diagnostics
3. **Position Tracking**: Every token and AST node tracks source location
4. **Type Safety**: Type system enforced throughout
5. **Memory Efficient**: Minimal allocation overhead
6. **Extensible**: Easy to add new operators, keywords, types

## References

- **EBNF Grammar**: Available in grammar/ directory
- **Language Specification**: See spec/ directory
- **Examples**: In examples/frontend/ directory

## License

Part of the Vitte Compiler project - MIT License
