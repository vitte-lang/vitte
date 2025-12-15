# Architecture

- support/: containers + filesystem + arena
- diag/: source_map + diagnostic + emitters
- front/: lexer + tokens (+ parser stub)
- back/: emit C minimal
- pack/: placeholder Muffin graph (future)

Le compilateur actuel vise un sous-ensemble "bootstrap-friendly":
- blocs `.end` (skip statements)
- `module X`, `import`, `export` (tokenisés)
- `fn name(...) -> type` (détection naïve)
