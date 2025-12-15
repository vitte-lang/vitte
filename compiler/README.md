# vittec (C)

Compilateur Vitte en C (monolith, pas de multi-stage).

Objectifs:
- binaire unique `vittec`
- pipeline: read -> lex -> (parse stub) -> emit C (bootstrap-friendly)
- diagnostics human/json
- build sans dépendances externes (C11)

Build rapide:
- `./scripts/build.sh`
- `./scripts/run_tests.sh`
