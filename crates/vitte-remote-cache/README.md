

# vitte-remote-cache

Système **de cache distant distribué** pour l’écosystème Vitte.  
`vitte-remote-cache` permet le stockage, la récupération et la synchronisation des artefacts compilés, métadonnées et analyses sur des serveurs distants afin d’accélérer les builds collaboratifs et CI/CD.

---

## Objectifs

- Offrir un **cache distribué performant et résilient**.  
- Supporter plusieurs backends : HTTP, S3, Redis, SSH, et filesystem distant.  
- Intégration directe avec `vitte-build`, `vitte-cache`, `vitte-compiler` et `vitte-docgen`.  
- Gestion intelligente des invalidations et des priorités de récupération.  
- Compatibilité complète avec les environnements CI (GitHub Actions, GitLab CI, etc.).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `client`      | Gestion des requêtes vers les serveurs distants |
| `server`      | Implémentation d’un serveur de cache distant |
| `protocol`    | Définition du protocole de communication (RPC/HTTP/JSON) |
| `auth`        | Authentification et gestion des jetons sécurisés |
| `policy`      | Règles de rétention, priorités et quotas |
| `sync`        | Synchronisation bidirectionnelle entre caches locaux et distants |
| `tests`       | Tests d’intégration et de cohérence réseau |

---

## Exemple d’utilisation

### Démarrage du serveur distant

```bash
vitte remote-cache serve --port 8080 --dir /data/vitte-cache
```

### Configuration du client

```toml
# .vitte/config.toml
[remote-cache]
url = "https://cache.vitte.dev"
token = "${VITTE_TOKEN}"
backend = "s3"
```

### Utilisation en compilation

```bash
vitte build --use-remote-cache
```

---

## Intégration

- `vitte-cache` : synchronisation transparente entre cache local et distant.  
- `vitte-build` : récupération automatique des artefacts.  
- `vitte-compiler` : enregistrement des métadonnées et binaires.  
- `vitte-docgen` : cache du rendu HTML/JSON partagé.  
- `vitte-cli` : configuration via `vitte remote-cache`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-cache = { path = "../vitte-cache", version = "0.1.0" }

axum = "0.7"
tokio = { version = "1", features = ["full"] }
reqwest = { version = "0.12", features = ["json", "stream"] }
s3 = "0.35"
redis = { version = "0.25", features = ["tokio-comp"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tracing = "0.1"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `hyper` pour backends HTTP custom, `rustls` pour TLS natif, `chrono` pour politiques temporelles.

---

## Tests

```bash
cargo test -p vitte-remote-cache
```

- Tests de cohérence entre cache local et distant.  
- Tests de performance de synchronisation.  
- Tests d’authentification et de tolérance aux pannes réseau.  
- Benchmarks de débit en environnement CI/CD.

---

## Roadmap

- [ ] Support complet du backend S3 multipart.  
- [ ] Déduplication intelligente des artefacts identiques.  
- [ ] Chiffrement transparent au repos et en transit.  
- [ ] Mode cluster haute disponibilité.  
- [ ] Interface graphique de gestion via `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau