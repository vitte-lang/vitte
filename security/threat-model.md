# Threat Model — Project Name

> **Version** : 1.0  
> **Date** : 2025-08-15  
> **Auteur** : Équipe Sécurité  
> **Confidentialité** : Interne

---

## 1. Introduction

Ce document décrit le **modèle de menace** appliqué à _Project Name_ dans le but :
- d’identifier les risques de sécurité,
- d’évaluer leur impact et leur probabilité,
- de définir des mesures d’atténuation réalistes et efficaces.

Méthodologie : **STRIDE** (Microsoft), complétée par **DREAD** pour l’évaluation.

---

## 2. Portée

Le modèle couvre :
- L’application principale (backend, frontend, CLI, API)
- Les dépendances logicielles et bibliothèques externes
- Les environnements d’exécution (serveurs, CI/CD, postes développeurs)
- Les flux de données (utilisateur → API → base de données)
- Les intégrations tierces (paiement, authentification, stockage)

Hors scope :
- Menaces physiques sur les data centers non gérés directement
- Attaques étatiques ciblées de niveau APT ultra-sophistiquées

---

## 3. Architecture du Système

**Composants :**
- **Frontend** (web/mobile) — interface utilisateur
- **API Gateway** — point d’entrée sécurisé, authentification
- **Service Backend** — logique métier
- **Base de données** — stockage persistant
- **CI/CD** — pipeline de déploiement
- **Monitoring / Logging** — détection d’incidents

**Flux principaux :**
1. Utilisateur → HTTPS → Frontend
2. Frontend → HTTPS → API Gateway
3. API Gateway → gRPC/HTTPS → Backend
4. Backend → SQL/NoSQL → Base de données

---

## 4. Méthodologie STRIDE

| Menace | Description | Impact | Exemples |
|--------|-------------|--------|----------|
| **S**poofing | Usurpation d'identité | Élevé | Vol de token JWT, attaque MITM |
| **T**ampering | Altération de données | Élevé | Modification de payload API |
| **R**epudiation | Négation d’action | Moyen | Absence de logs horodatés fiables |
| **I**nformation Disclosure | Fuite d’informations | Élevé | Fuite DB via injection SQL |
| **D**enial of Service | Indisponibilité | Élevé | Saturation API par requêtes massives |
| **E**levation of Privilege | Escalade de privilèges | Critique | Prise de contrôle compte admin |

---

## 5. Identification des Actifs

| Actif | Description | Sensibilité |
|-------|-------------|-------------|
| Données utilisateur | PII, mots de passe, emails | Élevée |
| Clés API | Accès aux services internes | Élevée |
| Code source | IP propriétaire | Élevée |
| Pipeline CI/CD | Contrôle du déploiement | Élevée |
| Secrets d’infrastructure | Accès à DB, stockage, cloud | Critique |

---

## 6. Menaces et Risques

| ID | Menace | Vecteur | Probabilité | Impact | Score (DREAD) | Niveau |
|----|--------|---------|-------------|--------|---------------|--------|
| T1 | Injection SQL | Input non filtré | Moyenne | Élevé | 8/10 | Haut |
| T2 | XSS stocké | Formulaire mal protégé | Moyenne | Élevé | 7/10 | Haut |
| T3 | Vol de token JWT | Interception réseau | Faible | Critique | 6/10 | Haut |
| T4 | DoS API | Requêtes massives | Élevée | Élevé | 9/10 | Critique |
| T5 | Escalade CI/CD | Vol de credentials | Moyenne | Critique | 9/10 | Critique |

---

## 7. Contre-mesures

### 7.1 Authentification & Autorisation
- Utiliser OAuth2.0 / OpenID Connect
- JWT signés avec rotation de clés
- MFA obligatoire pour accès sensibles

### 7.2 Validation & Filtrage
- Filtrage côté serveur **ET** côté client
- Requêtes préparées pour SQL
- Encode/désinfecter les données affichées

### 7.3 Sécurité réseau
- Forcer HTTPS/TLS 1.3
- HSTS + CSP strict
- Pare-feu applicatif (WAF)

### 7.4 Sécurité CI/CD
- Secrets stockés dans un coffre (Vault)
- Build reproductibles et signés
- Analyse SAST/DAST automatisée

### 7.5 Surveillance & Réponse
- Logging centralisé (immutabilité)
- SIEM avec alertes temps réel
- Playbook d’incident documenté

---

## 8. Plan de Réponse à Incident

1. **Détection** : identification d’activité anormale
2. **Confinement** : limiter propagation
3. **Éradication** : supprimer menace
4. **Restauration** : remettre en service
5. **Post-mortem** : analyse et amélioration

---

## 9. Gestion des Mises à Jour

- Patch critique appliqué en < 24h
- Patch normal en < 7 jours
- Mises à jour automatiques possibles sur dépendances non-critiques

---

## 10. Conclusion

Ce modèle de menace doit être revu **tous les 6 mois** ou après un changement majeur d’architecture.  
La sécurité est un **processus continu**, non un état figé.

---

## 11. Annexes

- **Références** :
  - [OWASP Top 10](https://owasp.org/www-project-top-ten/)
  - [CWE](https://cwe.mitre.org/)
  - [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)
- **Glossaire** :
  - **PII** : Personally Identifiable Information
  - **MFA** : Multi-Factor Authentication
  - **SIEM** : Security Information and Event Management

