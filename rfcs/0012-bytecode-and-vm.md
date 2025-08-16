# RFC 0012 — Bytecode & Virtual Machine

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0012  

---

## Résumé
Spécifie le bytecode et la machine virtuelle Vitte (VM).

## Motivation
Un bytecode portable est nécessaire pour l’exécution multi-plateformes et les optimisations.

## Détails
### Structure d’un Chunk
- Constantes  
- Instructions  
- Table des symboles  

### Exemple d’opcodes
- `LoadConst idx`  
- `Add`  
- `Store idx`  
- `Call fn`  
- `Return`  

### Exécution
Stack-based VM.  

```text
push 2
push 3
add
print
```

## Alternatives
VM registre (plus complexe).  

## Impact
Impact fort sur perfs.  

## Adoption
MVP déjà implémenté.  

## Références
- Python bytecode  
- Lua VM  
- WebAssembly  
