# Vitte — Fuzz Suite (libFuzzer via cargo-fuzz)

Ce dossier fournit une batterie de **fuzzers** prêts à brancher sur Vitte (tokenizer, parser,
pretty-printer, loader, VM, disasm, asm, wasm…).

⚙️ Par défaut, les *harnesses* tentent d'appeler les APIs réelles de `vitte-core`.
Si ça ne compile pas d'emblée (API différente), active la **feature `shims`**
et complète les fonctions dans `fuzz/shims/lib.rs` pour adapter à ton code.

## Prérequis
```bash
rustup toolchain install nightly
cargo install cargo-fuzz
```

## Lancer un fuzz cible
```bash
cd fuzz
# exemple: tokenizer
cargo +nightly fuzz run tokenizer -- -dict=dicts/vitte.dict
```

## Lancer tout (scripts)
```bash
cd fuzz
./scripts/run-all.sh
```

## Cibles fournies
- `tokenizer` — lecture de source UTF-8 → lexing
- `parser` — parsing syntaxique
- `pretty` — parse + pretty-print idempotent(ish)
- `loader` — chargeur de bytecode (format .vbc)
- `disasm` — désassembleur bytecode
- `vm` — exécution VM interprète dans un sandbox
- `asm` — assembleur texte → bytecode
- `wasm_emitter` — backend Wasm (si présent)
- `ini`, `csv` — parsers std (si exposés côté Rust)

## Dicos & corpus
- `dicts/vitte.dict` — mots-clés, opérateurs, motifs utiles
- `corpora/<target>/` — graines minimales

## Conseils
- Active ASan/UBSan (par défaut avec libFuzzer).
- Fixe `-max_len` et `-timeout=5` pour stabiliser l'exécution.
- Utilise `minimize_crash=1` pour réduire les inputs.

Bonne chasse aux UB 🐛
