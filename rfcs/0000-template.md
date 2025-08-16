# RFC 0000 — Template officiel des RFCs Vitte

- Auteur : Vincent  
- Statut : Draft  
- Date : 2025-08-16  
- Numéro : 0000  

---

## Résumé (Abstract)
Ce document définit la structure standard d’une RFC (Request For Comments) du langage **Vitte**.  
Toutes les propositions futures devront respecter ce modèle afin d’assurer clarté, cohérence et traçabilité.

## Motivation
Le langage Vitte vise à devenir un écosystème robuste. Chaque proposition doit être présentée sous forme d’un document normalisé pour assurer cohérence et relecture efficace.

## Détails de conception
Une RFC contient : Résumé, Motivation, Détails de conception, Alternatives, Impact, Adoption, Références.  
Un en-tête standard (Auteur, Date, Numéro, Statut) est obligatoire.

## Exemple
```markdown
# RFC 000X — Nom
- Auteur : …
- Statut : Proposed
- Date : YYYY-MM-DD
- Numéro : 000X
```

## Alternatives
- Ne rien imposer (rejeté).  
- Copier PEPs (rejeté, pas adapté).  

## Impact
Structure claire, automatisation possible.  

## Adoption
Immédiat.  

## Références
- Rust RFC Template  
- Python PEP 12  
- C++ WG21 proposals  
