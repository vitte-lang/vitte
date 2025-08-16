# RFC 0015 — IDE and LSP Support

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0015  

---

## Résumé
Spécifie le support IDE via **Language Server Protocol (LSP)**.

## Motivation
Les développeurs attendent autocomplétion, diagnostics, refactoring.

## Détails
- Implémentation LSP officielle.  
- Fonctions : completion, diagnostics, rename, goto definition.  
- Intégration VSCode, Vim, Emacs.  

### Exemple JSON LSP
```json
{ "method": "textDocument/completion", "params": { "textDocument": {"uri": "file:///main.vitte"} } }
```

## Alternatives
Pas de LSP (IDE dépendants).  

## Impact
Accroît adoption.  

## Adoption
Preview avec VSCode plugin.  

## Références
- Microsoft LSP spec  
- Rust Analyzer  
