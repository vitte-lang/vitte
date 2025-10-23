

# vitte-io

Bibliothèque **d’entrée/sortie bas-niveau** pour le langage Vitte.  
`vitte-io` fournit les primitives de lecture, écriture, buffering et streaming utilisées par le runtime, le compilateur et les outils CLI.

---

## Objectifs

- Offrir une couche I/O performante, compatible **sync** et **async**.  
- Gérer les fichiers, répertoires, sockets et flux mémoire.  
- Supporter les encodages texte/binaire (UTF-8, UTF-16, bin).  
- Intégration directe avec `vitte-runtime`, `vitte-cli` et `vitte-http`.  
- Fournir un modèle extensible pour les backends (FS, mémoire, réseau).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `fs`          | Gestion des fichiers et répertoires |
| `buffer`      | Buffers de lecture/écriture et I/O mémoire |
| `stream`      | Flux asynchrones et pipelines de données |
| `net`         | Sockets TCP/UDP et canaux réseau |
| `pipe`        | Communication entre processus et I/O anonymes |
| `encoding`    | Conversion et détection d’encodage |
| `async`       | Primitives I/O basées sur Tokio |
| `tests`       | Tests de performance et fiabilité I/O |

---

## Exemple d’utilisation

```rust
use vitte_io::fs::File;
use std::io::Write;

fn main() -> anyhow::Result<()> {
    let mut file = File::create("output.txt")?;
    file.write_all(b"Bonjour, Vitte!")?;
    Ok(())
}
```

---

## Exemple asynchrone

```rust
use vitte_io::stream::AsyncStream;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let mut stream = AsyncStream::connect("127.0.0.1:4000").await?;
    stream.send(b"ping").await?;
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : gestion centralisée des flux I/O.  
- `vitte-cli` : lecture interactive et redirection stdout/stderr.  
- `vitte-http` : support des requêtes et réponses réseau.  
- `vitte-analyzer` : lecture des fichiers sources et binaires.  
- `vitte-docgen` : écriture des rapports HTML et JSON.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

tokio = { version = "1", features = ["full"] }
bytes = "1.6"
encoding_rs = "0.8"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `mio` pour backend bas-niveau, `flate2` pour I/O compressé.

---

## Tests

```bash
cargo test -p vitte-io
```

- Tests d’ouverture et d’écriture fichiers.  
- Tests de flux asynchrones.  
- Tests d’encodage/décodage texte.  
- Benchmarks lecture/écriture multi-thread.

---

## Roadmap

- [ ] Support des pipes nommés (Windows, Unix).  
- [ ] Compression et chiffrement transparents.  
- [ ] Virtualisation du système de fichiers (in-memory FS).  
- [ ] Support complet des flux WebSocket.  
- [ ] Mode sandboxé pour builds sécurisés.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau