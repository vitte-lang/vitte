# RFC 0012 — Bytecode and VM

## Résumé
Ce RFC définit le format de **bytecode** du langage **Vitte** et la conception de la **machine virtuelle (VM)** qui l’exécute.  
Le bytecode est conçu pour être portable, compact et efficace, tout en préservant la sûreté et la détection d’erreurs.

## Motivation
Un bytecode dédié offre plusieurs avantages :  
- Portabilité entre architectures (x86, ARM, RISC-V, WASM).  
- Rapidité de chargement et d’exécution.  
- Simplification des optimisations du compilateur.  
- Support de l’outillage : debugger, profiler, analyse statique.  

## Design proposé

### Format du bytecode
- **Fichier binaire** avec en-tête :  
```
magic   : u32   // "VBC0"
version : u16   // version du format
flags   : u16
```
- **Sections** :  
  - `const_pool` : constantes (nombres, chaînes, fonctions).  
  - `code` : suite d’instructions.  
  - `debug` : infos optionnelles (fichiers, lignes).  

### Jeu d’instructions (MVP)
- **Chargement/stockage** : `LoadConst`, `LoadLocal`, `StoreLocal`.  
- **Arithmétique** : `Add`, `Sub`, `Mul`, `Div`, `Mod`.  
- **Logique** : `And`, `Or`, `Not`, `CmpEq`, `CmpLt`, `CmpGt`.  
- **Contrôle** : `Jump`, `JumpIf`, `Call`, `Return`.  
- **Mémoire** : `Alloc`, `Free`, `LoadPtr`, `StorePtr`.  
- **Structures** : `NewStruct`, `GetField`, `SetField`.  

Exemple (pseudo-bytecode) :
```
0: LoadConst 0   // 42
1: StoreLocal 0
2: LoadLocal 0
3: LoadConst 1   // 1
4: Add
5: Return
```

### Machine virtuelle
- **Stack-based** : instructions opèrent sur une pile.  
- **Registres internes** pour optimisation (hybride stack/register).  
- **Garantie de sûreté** : vérification des types et bornes avant exécution.  

### Gestion mémoire
- **Ownership model** respecté dans la VM.  
- **GC optionnel** pour certaines cibles (interop).  
- Allocation/désallocation explicite possible (`Alloc` / `Free`).  

### Multithreading
- VM supporte plusieurs instances isolées.  
- Communication via canaux (`Channel`).  
- Futures et async intégrés au bytecode (`Await`, `Spawn`).  

### Debugging et introspection
- Section `debug` contient : mapping adresse → ligne de source.  
- Commandes de VM : `--trace`, `--disasm`, `--profile`.  
- Intégration avec LSP/IDE pour step-by-step debugging.  

### Exemple d’utilisation
Compilation d’un fichier :  
```sh
vitc main.vit --target=vm -o main.vbc
```

Exécution :  
```sh
vitvm main.vbc
```

### Extensions futures
- **JIT** (Just-In-Time) pour accélérer certaines cibles.  
- **WASM bridge** : conversion vers WebAssembly.  
- **Sandboxing** : exécution sécurisée en environnement restreint.  

## Alternatives considérées
- **Interprétation directe de l’AST** : rejetée pour performance insuffisante.  
- **LLVM IR comme bytecode unique** : rejeté car trop bas niveau pour l’outillage.  
- **Bytecode spécifique à une plateforme** : rejeté pour préserver la portabilité.  

## Impact et compatibilité
- Impact fort : la VM est une brique essentielle de l’écosystème Vitte.  
- Compatible avec les RFC sur ownership, async et FFI.  
- Assure un runtime portable pour serveurs, desktop et embarqué.  
- Prépare l’avenir avec le JIT et l’intégration WASM.  

## Références
- [JVM Specification](https://docs.oracle.com/javase/specs/jvms/se11/html/)  
- [Python Bytecode](https://docs.python.org/3/library/dis.html)  
- [WebAssembly](https://webassembly.org/)  
- [Lua VM](https://www.lua.org/manual/5.4/manual.html#4)  
