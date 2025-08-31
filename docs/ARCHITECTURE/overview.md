# Architecture — Vue d’ensemble

```mermaid
flowchart LR
  src[Source .vit] --> LEXER --> PARSER --> AST --> IR[IR Vitte (CFG/DFG)]
  IR -->|analyse/passes| VITBC[Émetteur VITBC]
  VITBC --> VM[VM / Runtime]
  subgraph IDE
    LSP
  end
  LSP -->|parse incrémental| AST
  CLI[CLI vitte-*] -->|wrap| LEXER
  CLI --> PARSER
  CLI --> VM
```

- **Lexer/Parser/AST** : front-end classique, erreurs jolies, property-tests.
- **IR** : graph-orienté (CFG/DFG), passes et validations, export DOT.
- **VITBC** : conteneur bytecode versionné, sections & CRC32, compression optionnelle.
- **VM** : interpréteur pur, snapshots (serde), exécution deterministe.
- **Runtime** : I/O optionnels, loaders, REPL/CLI.
- **LSP** : diagnostics, complétion, hover, go-to.
- **Tools** : fmt/check/pack/dump/graph/run.

Design sans `unsafe` (forbid), features explicites : `std`, `alloc-only`, `serde`, `tracing`, etc.
