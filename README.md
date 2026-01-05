# Vitte Language Architecture - Option 3

Welcome to the **Vitte Programming Language** repository !

This is the official repository for Vitte, organized into 6 main domains reflecting our language architecture philosophy.

## 🏗️ Architecture

```
vitte/
├── foundation/      Core runtime foundations
├── lingua/          Language definition & type system
├── compiler/        Compilation engine
├── runtime/         Standard library & runtime
├── tools/           Development tools
├── docs/            Documentation & specifications
└── src/             Build infrastructure
```

### foundation/
**Core runtime foundations and utilities**

- **core/** : Core language components and error handling
- **utils/** : Data structures, memory management, utilities

### lingua/
**Language definition, syntax, and type system**

- **syntax/** : Lexer, parser, and AST (Vitte frontend)
- **types/** : Type system, type checking, semantic analysis

### compiler/
**Compilation engine and code generation**

- **ir/** : Intermediate representations and optimization
- **backends/** : Code generation targets (LLVM, GCC, Cranelift)

### runtime/
**Standard library and runtime support**

- **std/** : Complete standard library, core lib, allocators

### tools/
**Development tools and utilities**

- **vittec/** : Main Vitte compiler executable
- *vittedoc/* : Documentation generator (in src/runtime/vittedoc)
- *CLI tools* : Additional development utilities

### docs/
**Language documentation and specifications**

- Specification and language reference
- Developer guides and tutorials
- API documentation

### src/
**Build infrastructure and development support**

- Build system, CI/CD, bootstrap compiler
- External dependencies management

## 🚀 Quick Start

```bash
cd vitte
./bootstrap.sh          # Build the compiler
./build.sh              # Build everything
./test.sh               # Run tests
```

## 📖 Documentation

- [Language Specification](docs/spec/grammar.md)
- [Developer Guide](docs/dev-guide/)
- [API Reference](docs/api/)

## 🤝 Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

## 📄 License

[License details here]

---

**Vitte: A language designed for clarity, performance, and authentic expression.**
