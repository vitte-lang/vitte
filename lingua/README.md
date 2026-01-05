# Lingua - Vitte Language Definition

The language layer defining Vitte's syntax, semantics, and type system.

## Structure

### syntax/
Language parsing and abstract syntax tree
- **vitte_lexer**: Tokenization and lexical analysis
- **vitte_parse**: Parser implementation
- **vitte_ast**: Abstract Syntax Tree definitions
- **vitte_expand**: Macro expansion
- **vitte_macros**: Built-in macros
- And more...

### types/
Type system and semantic analysis
- **vitte_hir**: High-level Intermediate Representation
- **vitte_typeck**: Type checking implementation
- **vitte_infer**: Type inference engine
- **vitte_borrowck**: Borrow checker
- **vitte_trait_selection**: Trait resolution
- And more...

## Purpose

The lingua layer defines what Vitte is - its syntax rules, type system, and semantic guarantees.
