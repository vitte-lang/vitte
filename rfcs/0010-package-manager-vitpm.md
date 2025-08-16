# RFC 0010 — Package Manager (vitpm)

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0010  

---

## Résumé
Définit le gestionnaire de paquets officiel **vitpm**.  

## Motivation
Partager, versionner et distribuer les bibliothèques Vitte.  

## Détails
### Fichier manifeste
`vitte.toml`
```toml
[package]
name = "hello-vitte"
version = "0.1.0"

[dependencies]
http = "1.2"
```

### Commandes
- `vitpm init` → créer un projet  
- `vitpm build` → compiler avec deps  
- `vitpm publish` → publier sur registre  
- `vitpm install foo` → installer un paquet  

## Alternatives
Pas de gestionnaire intégré (difficile à adopter).  

## Impact
Indispensable pour l’écosystème.  

## Adoption
Phase alpha avec registre central.  

## Références
- Cargo (Rust)  
- npm (JS)  
- pip (Python)  
