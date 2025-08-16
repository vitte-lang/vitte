# RFC 0013 — Security Threat Model

- Auteur : Vincent  
- Statut : Draft  
- Date : 2025-08-16  
- Numéro : 0013  

---

## Résumé
Établit un modèle de menaces de sécurité pour Vitte.

## Motivation
Un langage moderne doit intégrer sécurité par défaut (sandboxing, memory safety).

## Détails
### Menaces considérées
- Buffer overflows.  
- Code injection via FFI.  
- Attaques DoS (boucles infinies).  

### Mesures
- Ownership model.  
- Vérification FFI stricte.  
- Limites d’exécution VM.  

## Alternatives
Laisser à l’utilisateur (rejeté).  

## Impact
Améliore crédibilité du langage.  

## Adoption
Intégré en continu.  

## Références
- Rustonomicon security section  
- OWASP top 10  
