

# vitte-http

Bibliothèque **HTTP asynchrone et typée** pour l’écosystème Vitte.  
`vitte-http` fournit les primitives réseau, les clients et serveurs HTTP(S), et les abstractions nécessaires à la communication entre outils (`vitte-docgen`, `vitte-lsp`, etc.).

---

## Objectifs

- Offrir une **API HTTP moderne**, asynchrone et performante.  
- Supporter HTTP/1.1, HTTP/2 et HTTP/3 (QUIC).  
- Fournir des clients et serveurs unifiés avec gestion des en-têtes, cookies et compression.  
- Intégration directe avec `vitte-runtime` pour exécution parallèle.  
- Support des formats JSON, multipart et flux binaires.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `client`      | Client HTTP asynchrone, requêtes GET/POST/PUT/DELETE |
| `server`      | Serveur HTTP minimal, gestion des routes et middlewares |
| `headers`     | Gestion des en-têtes standards et personnalisés |
| `body`        | Représentation des corps de requête et réponse |
| `router`      | Routage dynamique et correspondance des chemins |
| `json`        | Sérialisation/désérialisation JSON intégrée |
| `stream`      | Support des flux asynchrones et WebSocket |
| `tls`         | Support HTTPS via `rustls` |
| `tests`       | Tests d’intégration client/serveur |

---

## Exemple d’utilisation

```rust
use vitte_http::client::HttpClient;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let client = HttpClient::new();
    let response = client.get("https://api.github.com/repos/vitte-lang/vitte").await?;

    println!("Statut: {}", response.status());
    println!("Corps: {}", response.text().await?);
    Ok(())
}
```

---

## Exemple de serveur

```rust
use vitte_http::server::{HttpServer, Response};

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let mut server = HttpServer::new("127.0.0.1:8080");

    server.route("/hello", |req| async move {
        Response::text(format!("Hello {}!", req.remote_addr()))
    });

    server.run().await?;
    Ok(())
}
```

---

## Intégration

- `vitte-docgen` : téléchargement et publication de documentation.  
- `vitte-lsp` : communication client/serveur via HTTP et WebSocket.  
- `vitte-build` : requêtes HTTP pour téléchargement distant de dépendances.  
- `vitte-studio` : API locale pour communication IDE ↔ runtime.  
- `vitte-cli` : requêtes REST vers services externes.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

reqwest = { version = "0.12", features = ["json", "multipart", "cookies", "gzip"] }
hyper = { version = "1", features = ["full"] }
tokio = { version = "1", features = ["full"] }
rustls = "0.22"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `warp` ou `axum` pour routage étendu, `tower` pour middlewares.

---

## Tests

```bash
cargo test -p vitte-http
```

- Tests unitaires client/serveur.  
- Tests TLS et connexions persistantes.  
- Tests de charge et benchmarks HTTP/2.  
- Validation des en-têtes et flux JSON.

---

## Roadmap

- [ ] Support complet HTTP/3 (QUIC).  
- [ ] Compression Brotli/Zstd intégrée.  
- [ ] Cache local des réponses.  
- [ ] Serveur WebSocket avancé avec rooms.  
- [ ] Intégration directe avec `vitte-runtime` et `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau