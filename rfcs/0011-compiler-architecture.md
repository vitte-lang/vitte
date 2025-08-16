# RFC 0011 — Compiler Architecture

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0011  

---

## Résumé
Décrit l’architecture générale du compilateur Vitte (front-end, middle-end, back-end).

## Motivation
Une architecture claire facilite maintenance, extensions et optimisation.

## Détails
### Étapes du compilateur
1. **Lexing** → transforme source en tokens.  
2. **Parsing** → AST.  
3. **Analyse sémantique** → typage, ownership.  
4. **Middle-end** → optimisations.  
5. **Back-end** → génération bytecode ou binaire natif.  

### Exemple
```
src.vitte -> tokens -> AST -> HIR -> MIR -> bytecode/LLVM IR
```

### Organisation
- `vitte-compiler` crate : front-end + middle.  
- `vitte-vm` crate : exécution bytecode.  
- `vitte-core` crate : structures partagées.  

## Alternatives
Monolithique (difficile à maintenir).  

## Impact
Structure modulaire claire.  

## Adoption
Déjà partiellement en place.  

## Références
- Rustc architecture  
- LLVM design  
- Clang pipeline  
