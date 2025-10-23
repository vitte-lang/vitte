

# vitte-tls

Bibliothèque **de chiffrement et transport sécurisé TLS/SSL** pour l’écosystème Vitte.  
`vitte-tls` fournit les primitives cryptographiques, les sessions sécurisées et les outils de gestion de certificats nécessaires pour les communications réseau chiffrées.

---

## Objectifs

- Offrir une **implémentation performante et sécurisée du protocole TLS 1.3**.  
- Supporter les certificats X.509, la vérification de chaîne et le stockage des clés.  
- Intégration directe avec `vitte-net`, `vitte-http`, `vitte-runtime` et `vitte-studio`.  
- Fournir une API unifiée pour le client et le serveur.  
- Garantir la compatibilité avec OpenSSL et rustls.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `session`     | Gestion des sessions TLS (client/serveur) |
| `cert`        | Chargement et vérification des certificats X.509 |
| `key`         | Gestion des clés privées/publiques et formats PEM/DER |
| `handshake`   | Établissement et négociation des connexions sécurisées |
| `cipher`      | Implémentation des suites cryptographiques et algorithmes |
| `tests`       | Validation de sécurité, compatibilité et performances |

---

## Exemple d’utilisation

```rust
use vitte_tls::{TlsClient, TlsConfig};
use std::net::TcpStream;

fn main() -> anyhow::Result<()> {
    let config = TlsConfig::default();
    let stream = TcpStream::connect("example.com:443")?;
    let mut client = TlsClient::connect(stream, config)?;

    client.write_all(b"GET / HTTP/1.1\r\nHost: example.com\r\n\r\n")?;
    let response = client.read_response()?;

    println!("{}", response);
    Ok(())
}
```

---

## Intégration

- `vitte-net` : couche de transport TCP sécurisée.  
- `vitte-http` : communication HTTPS client/serveur.  
- `vitte-runtime` : gestion asynchrone des connexions TLS.  
- `vitte-build` : vérification des dépendances de sécurité.  
- `vitte-studio` : affichage des diagnostics SSL/TLS.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-net = { path = "../vitte-net", version = "0.1.0" }

rustls = "0.23"
webpki-roots = "0.26"
rcgen = "0.12"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `openssl` pour compatibilité bas niveau, `native-tls` pour intégration avec les bibliothèques système.

---

## Tests

```bash
cargo test -p vitte-tls
```

- Tests de handshake client/serveur.  
- Tests de vérification des certificats.  
- Tests de compatibilité rustls/OpenSSL.  
- Benchmarks de performance sur transferts chiffrés.

---

## Roadmap

- [ ] Support complet TLS 1.4 (prévision post-2026).  
- [ ] OCSP et stapling automatique.  
- [ ] Gestion de sessions persistantes et reprise.  
- [ ] Intégration avec `vitte-cloud` pour certificats distants.  
- [ ] Audit complet des algorithmes cryptographiques.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau