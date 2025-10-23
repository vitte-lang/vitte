

# vitte-net

Bibliothèque **réseau asynchrone** du langage Vitte.  
`vitte-net` fournit les primitives TCP, UDP, HTTP et WebSocket utilisées par le runtime, le compilateur distribué et les outils en ligne de commande.

---

## Objectifs

- Offrir une **API réseau moderne et multiplateforme**.  
- Supporter TCP, UDP, WebSocket et HTTP/HTTPS.  
- Fournir des abstractions de haut niveau pour les flux et sockets.  
- Intégration directe avec `vitte-runtime` et `vitte-http`.  
- Garantir la compatibilité avec les environnements asynchrones (`tokio`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `tcp`         | Gestion des connexions TCP (serveur/client) |
| `udp`         | Gestion des sockets datagrammes |
| `ws`          | Communication WebSocket full-duplex |
| `http`        | Support minimal HTTP intégré |
| `stream`      | Flux réseau typés et buffers asynchrones |
| `tls`         | Sécurité via TLS (rustls) |
| `resolver`    | Résolution DNS asynchrone |
| `tests`       | Tests d’intégration réseau et de performance |

---

## Exemple d’utilisation

### Client TCP

```rust
use vitte_net::tcp::TcpClient;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let mut client = TcpClient::connect("example.org:80").await?;
    client.send(b"GET / HTTP/1.1\r\nHost: example.org\r\n\r\n").await?;
    let response = client.recv().await?;
    println!("{}", String::from_utf8_lossy(&response));
    Ok(())
}
```

### Serveur TCP

```rust
use vitte_net::tcp::TcpServer;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let mut server = TcpServer::bind("127.0.0.1:8080").await?;

    while let Ok((mut socket, addr)) = server.accept().await {
        tokio::spawn(async move {
            println!("Connexion de {}", addr);
            socket.send(b"Bienvenue sur VitteNet!\n").await.ok();
        });
    }
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : gestion des IO réseau et threads asynchrones.  
- `vitte-http` : implémentation complète du protocole HTTP(S).  
- `vitte-lsp` : communication client/serveur JSON-RPC.  
- `vitte-docgen` : téléchargement et publication de documentation.  
- `vitte-studio` : synchronisation réseau IDE ↔ runtime.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

bytes = "1.6"
tokio = { version = "1", features = ["full"] }
async-trait = "0.1"
rustls = "0.22"
webpki-roots = "0.26"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `tokio-tungstenite` pour WebSocket, `trust-dns-resolver` pour DNS avancé.

---

## Tests

```bash
cargo test -p vitte-net
```

- Tests TCP/UDP/WebSocket.  
- Tests de connexion sécurisée TLS.  
- Tests d’intégration avec `vitte-runtime`.  
- Benchmarks de latence et débit.

---

## Roadmap

- [ ] Support QUIC/HTTP3.  
- [ ] Mode P2P et communication inter-noeuds.  
- [ ] Support complet WebSocket binaire et texte.  
- [ ] Compression réseau adaptative.  
- [ ] Intégration avec le profiler `vitte-metrics`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau