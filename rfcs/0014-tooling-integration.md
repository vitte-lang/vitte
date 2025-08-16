# RFC 0014 — Tooling Integration

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0014  

---

## Résumé
Décrit l’intégration de Vitte avec les outils (build, CI, docs).

## Motivation
Un langage ne vit que s’il est intégré dans les workflows dev.

## Détails
- `vitpm` : gestion paquets.  
- `vitte-doc` : génération docs.  
- `vitte-fmt` : formateur officiel.  
- `vitte-lsp` : serveur de langage pour IDE.  

## Exemple
```bash
vitte-fmt main.vitte
vitte-doc --open
```

## Alternatives
Outils tiers non officiels (fragmentation).  

## Impact
Favorise adoption.  

## Adoption
Release 1.0 avec bundle complet.  

## Références
- Cargo / rustfmt / rustdoc  
- Go tooling  
