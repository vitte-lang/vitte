# RFC 0011 — Compiler Architecture

## Résumé
Ce RFC définit l’architecture du **compilateur Vitte** (`vitc`).  
Il décrit les différentes étapes de compilation, la structure interne du compilateur et ses interfaces, ainsi que les objectifs de performance et de portabilité.

## Motivation
Un compilateur clair et modulaire est indispensable pour :  
- assurer la robustesse et la maintenabilité,  
- faciliter l’ajout futur d’optimisations et de backends,  
- permettre la portabilité vers différentes cibles (natif, VM, WASM, embarqué),  
- fournir une base pour les outils annexes (LSP, IDE, débogueur).  

## Design proposé

### Étapes de compilation
Le compilateur suit une pipeline modulaire :

```
src.vit
   │
   ▼
[1] Lexing (tokenizer)
   │
   ▼
[2] Parsing (AST)
   │
   ▼
[3] Analyse sémantique
   │   ├─ vérification des types
   │   ├─ borrow checker (ownership, lifetimes)
   │   └─ résolution des noms et imports
   │
   ▼
[4] IR intermédiaire (HIR → MIR)
   │
   ▼
[5] Génération de bytecode Vitte (VBC)
   │
   ▼
[6] Optimisations (inlining, DCE, SSA)
   │
   ▼
[7] Backend natif (LLVM/Cranelift)
   │        ou
   ▼
[8] Génération VM / WASM
```

### Modules internes
- **Lexer** : transforme le texte source en tokens.  
- **Parser** : construit un AST structuré.  
- **Type Checker** : assure la cohérence des types.  
- **Borrow Checker** : applique les règles d’ownership.  
- **IR** : représentations intermédiaires (`HIR`, `MIR`).  
- **Codegen** : génère le bytecode ou le natif.  
- **Optimizer** : applique des passes optimisantes.  
- **Driver** : gère les options CLI, les erreurs, et l’orchestration.  

### Exemple de commande
```sh
vitc build main.vit --target=native --release
```

### Cibles supportées
- **Natif** : via LLVM/Cranelift.  
- **VM** : bytecode Vitte exécuté dans `vitvm`.  
- **WASM** : génération de modules WebAssembly.  
- **Embedded** : génération binaire minimaliste.  

### Gestion des erreurs
- Diagnostics structurés (codes, messages, notes).  
- Intégration avec LSP pour IDE.  
- Possibilité de `--explain <code>` pour détailler une erreur.  

### Optimisations prévues
- **Inlining** automatique.  
- **Dead Code Elimination**.  
- **Constant Folding**.  
- **SSA form** pour optimisation des registres.  
- **Loop unrolling** et **vectorisation** (SIMD).  

## Alternatives considérées
- **Un compilateur monolithique** : rejeté pour manque de modularité.  
- **Interpréteur uniquement** : rejeté pour performances insuffisantes.  
- **Backend unique (LLVM)** : rejeté pour garder flexibilité et indépendance.  

## Impact et compatibilité
- Impact critique : ce RFC définit l’outil central de Vitte.  
- Compatible avec l’écosystème (vitpm, stdlib, VM).  
- Prépare la portabilité vers OS variés (Linux, Windows, macOS, BSD, Embedded).  
- Rend possible les optimisations futures sans casser la syntaxe.  

## Références
- [Rustc Architecture](https://rustc-dev-guide.rust-lang.org/)  
- [LLVM Project](https://llvm.org/)  
- [Cranelift Codegen](https://github.com/bytecodealliance/wasmtime/blob/main/cranelift)  
- [GCC Internals](https://gcc.gnu.org/onlinedocs/gccint/)  
