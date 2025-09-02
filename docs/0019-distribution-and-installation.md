# RFC 0019 — Distribution and Installation

## Résumé
Ce RFC définit les méthodes de **distribution** et d’**installation** du langage **Vitte**, incluant le compilateur, la VM, la bibliothèque standard et les outils (`vitpm`, `vitfmt`, etc.).  
Il vise à fournir une expérience fluide, multiplateforme et sécurisée pour les développeurs.

## Motivation
Pour favoriser l’adoption, un langage doit être simple à installer :  
- un seul outil, facile à mettre à jour,  
- support multiplateforme (Linux, macOS, Windows, BSD, Embedded),  
- options pour environnements contraints (air-gapped, entreprise),  
- distribution sécurisée (signatures, checksum).  

## Design proposé

### Canaux de distribution
1. **Installateur officiel** :  
   - Script unique (`curl -sSL https://get.vitte.dev | sh`).  
   - Télécharge la dernière version stable.  

2. **Archives binaires** :  
   - `.tar.gz` / `.zip` pour chaque plateforme.  
   - Vérification via SHA256 + signature GPG.  

3. **Package managers** :  
   - Linux : `apt`, `dnf`, `pacman`, `nix`.  
   - macOS : `brew install vitte`.  
   - Windows : `winget install vitte` ou `choco install vitte`.  

4. **Docker images** :  
   - `docker pull vitte/lang:latest`.  
   - Idéal pour CI/CD.  

### Organisation des versions
- **Stable** : mises à jour régulières, rétrocompatibles.  
- **Nightly** : builds expérimentales.  
- **LTS** : versions maintenues long terme pour entreprises.  

### Mise à jour
```sh
vitpm self update
```

### Installation air-gapped
- Téléchargement manuel des archives.  
- `vitpm` supporte un **registry local**.  
- Vérification d’intégrité obligatoire.  

### Exemple : installation rapide
```sh
# Linux / macOS
curl -sSL https://get.vitte.dev | sh

# Windows (PowerShell)
iwr -useb https://get.vitte.dev/install.ps1 | iex
```

### Exemple : CI avec Docker
```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    container: vitte/lang:latest
    steps:
      - uses: actions/checkout@v4
      - run: vitpm build --release
```

## Alternatives considérées
- **Uniquement archives binaires** : rejeté pour manque d’ergonomie.  
- **Uniquement gestionnaire de paquets système** : rejeté pour lenteur de distribution.  
- **Pas de LTS** : rejeté car nécessaire pour entreprises.  

## Impact et compatibilité
- Impact fort : adoption simplifiée grâce à installation rapide.  
- Compatible avec environnements grand public, cloud et entreprise.  
- Sécurisé grâce aux signatures et checksums.  
- Introduit un coût d’infrastructure (hébergement du registre).  

## Références
- [Rustup](https://rustup.rs/)  
- [NVM](https://github.com/nvm-sh/nvm)  
- [Go Install](https://go.dev/doc/install)  
