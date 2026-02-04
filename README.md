vitte/
├── README.md
├── LICENSE
├── CMakeLists.txt
├── .editorconfig
├── .gitignore
│
├── docs/                         # Spécifications du langage
│   ├── grammar/
│   │   ├── vitte.ebnf
│   │   └── vitte.pest
│   ├── semantics/
│   └── abi.md
│
├── src/
│   ├── vitte/                    # 🟦 LANGAGE VITTE (PUR)
│   │   ├── grammar/              # Syntaxe officielle
│   │   ├── std/                  # Standard library Vitte
│   │   │   ├── core/
│   │   │   ├── alloc/
│   │   │   │   └── compiler-builtins/
│   │   │   │       ├── builtins-shim/
│   │   │   │       │   ├── builtins/
│   │   │   │       │   ├── config/
│   │   │   │       │   ├── link/
│   │   │   │       │   ├── lib.vit
│   │   │   │       │   └── mod.vit
│   │   │   │       └── builtins-test/
│   │   │   │           └── cases/
│   │   │   ├── math/
│   │   │   ├── io/
│   │   │   └── test/
│   │   │
│   │   └── runtime/              # Runtime Vitte (compilé avec le programme)
│   │       ├── abi/
│   │       ├── alloc/
│   │       ├── memory/
│   │       ├── panic/
│   │       └── startup/
│   │
│   └── compiler/                 # 🟥 COMPILATEUR (C++)
│       ├── frontend/
│       │   ├── lexer/
│       │   ├── parser/
│       │   └── diagnostics/
│       │
│       ├── ir/
│       │   ├── ast/
│       │   ├── hir/
│       │   ├── mir/
│       │   └── passes/
│       │
│       ├── backends/
│       │   ├── llvm/
│       │   ├── cranelift/
│       │   ├── c_backend/        # bootstrap minimal
│       │   └── wasm/
│       │
│       ├── linker/
│       │   ├── symbols.cpp
│       │   └── driver.cpp
│       │
│       └── driver/
│           └── vittec.cpp        # binaire compilateur
│
├── runtime/                      # Runtime prêt à linker
│   └── std/
│       └── alloc/
│
├── tests/
│   ├── frontend/
│   ├── ir/
│   ├── runtime/
│   ├── builtins/
│   └── e2e/
│
├── tools/
│   ├── vittec/                   # CLI compilateur
│   ├── vittefmt/
│   └── vitte-lsp/
│
├── toolchain/                    # Bootstrap & packaging
│   ├── stage0/                   # C/C++ minimal
│   ├── stage1/
│   ├── stage2/
│   └── scripts/
│
└── third_party/                  # Dépendances externes