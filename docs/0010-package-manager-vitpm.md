# RFC 0010 — Package Manager vitpm

## Résumé
Ce RFC définit le gestionnaire de paquets officiel de **Vitte**, nommé **`vitpm`** (Vitte Package Manager).  
Il a pour rôle de gérer les dépendances, versions, distributions, builds et publications de bibliothèques et applications écrites en Vitte.

## Motivation
Un écosystème de langage moderne ne peut exister sans un gestionnaire de paquets robuste :  
- faciliter l’import et la mise à jour des bibliothèques,  
- gérer les dépendances transitives et la résolution de versions,  
- permettre la distribution de projets via un registre centralisé,  
- intégrer le build et le déploiement dans une expérience cohérente.  

## Design proposé

### Commandes principales
```sh
vitpm init              # initialise un nouveau projet
vitpm add foo@1.2.0     # ajoute une dépendance
vitpm update            # met à jour les dépendances
vitpm build             # construit le projet
vitpm test              # exécute les tests
vitpm publish           # publie sur le registre
vitpm install           # installe une appli globale
```

### Fichier de configuration : `vitte.toml`
```toml
[package]
name = "mon_projet"
version = "0.1.0"
edition = "2025"
license = "MIT"

[dependencies]
serde = "1.0"
async-net = { version = "0.2", features = ["tls"] }

[dev-dependencies]
test-lib = "0.3"
```

### Résolution de dépendances
- Basée sur **SemVer** (`^1.2.3`, `~1.2`, `=1.2.5`).  
- Système de *lockfile* (`vitte.lock`) garantissant la reproductibilité des builds.  
- Détection des conflits de versions et suggestion de résolution.  

### Registre officiel
- Registre central : `registry.vitte.dev`.  
- Support des registres privés (entreprise).  
- Authentification via token.  
- Téléchargement sécurisé (TLS + signatures).  

### Intégration avec le compilateur
- `vitpm build` appelle automatiquement `vitc`.  
- Support du *workspace* multi-crates.  
- Optimisation du caching (`target/`, `~/.vitpm/cache`).  

### Gestion des versions
- `vitpm publish` vérifie que la version respecte SemVer.  
- Possibilité de marquer des versions comme `alpha`, `beta`, `rc`.  
- Dépréciation possible avec avertissement (`yanked`).  

### Exemples d’utilisation

#### Initialisation
```sh
$ vitpm init mon_app
Création de mon_app/
 ├── src/main.vit
 ├── vitte.toml
```

#### Ajout de dépendance
```sh
$ vitpm add log@^1.0
```

#### Publication
```sh
$ vitpm publish --token $VITPM_TOKEN
```

## Alternatives considérées
- **Pas de gestionnaire officiel** (comme C/C++) : rejeté car freine l’écosystème.  
- **Intégration uniquement au compilateur (Go)** : rejetée pour garder modularité et extensibilité.  
- **Gestionnaire externe non standardisé** : rejeté pour cohérence de l’écosystème.  

## Impact et compatibilité
- Impact fort : adoption rapide du langage grâce à un écosystème riche.  
- Compatible avec CI/CD et déploiement cloud/embarqué.  
- Introduit une dépendance à un registre central, mais possibilité de miroirs et registres privés.  

## Références
- [Cargo (Rust)](https://doc.rust-lang.org/cargo/)  
- [npm (JavaScript)](https://www.npmjs.com/)  
- [pip (Python)](https://pypi.org/project/pip/)  
- [Go Modules](https://go.dev/ref/mod)  
