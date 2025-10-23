

# vitte-crypto

Primitives cryptographiques et utilitaires de sécurité pour **Vitte**.

Ce crate regroupe les fonctions de hachage, signature, chiffrement, génération aléatoire et gestion de clés utilisées dans le compilateur, le runtime et les outils CLI de Vitte. Il fournit des abstractions sûres, auditées et performantes pour la cryptographie moderne.

---

## Objectifs

- Offrir des primitives cryptographiques fiables et vérifiables  
- Fournir une API haut niveau pour le hashing, la signature et le chiffrement  
- S’intégrer de manière transparente avec `vitte-core` et `vitte-io`  
- Respecter les normes modernes (SHA2, SHA3, BLAKE3, Ed25519, AES-GCM)  
- Garantir la compatibilité multiplateforme et l’isolation du RNG

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `hash`         | Hachage (SHA2, SHA3, BLAKE3, CRC32, Adler32) |
| `mac`          | HMAC, KMAC, Poly1305 |
| `rand`         | Génération pseudo-aléatoire sécurisée |
| `sym`          | Chiffrement symétrique (AES-GCM, ChaCha20-Poly1305) |
| `asym`         | Chiffrement et signature asymétriques (Ed25519, RSA) |
| `derive`       | Dérivation de clés (PBKDF2, HKDF, scrypt, Argon2) |
| `encode`       | Encodage Base64, Base58, hex, PEM |
| `fingerprint`  | Empreintes de clés et identifiants |
| `cli`          | Commandes de génération et vérification |
| `tests`        | Tests cryptographiques reproductibles |

---

## Exemple d’utilisation

### Hachage

```rust
use vitte_crypto::hash::blake3;

let digest = blake3::hash(b"Bonjour Vitte");
println!("Hash: {}", digest.to_hex());
```

### Signature

```rust
use vitte_crypto::asym::ed25519::{Keypair, Signature};

let kp = Keypair::generate();
let message = b"Vitte forever";
let sig = kp.sign(message);
assert!(kp.verify(message, &sig).is_ok());
```

### Chiffrement symétrique

```rust
use vitte_crypto::sym::aes_gcm;

let key = aes_gcm::Key::generate();
let nonce = aes_gcm::Nonce::random();
let ciphertext = aes_gcm::encrypt(&key, &nonce, b"secret").unwrap();
let plain = aes_gcm::decrypt(&key, &nonce, &ciphertext).unwrap();
assert_eq!(plain, b"secret");
```

---

## CLI

```
vitte crypto hash <fichier> [--algo sha3|blake3|sha256]
vitte crypto sign <fichier> --key <chemin>
vitte crypto verify <fichier> --sig <fichier.sig> --key <pubkey>
vitte crypto gen-key [--type ed25519|rsa]
```

Exemples :
```
vitte crypto hash Cargo.toml --algo blake3
vitte crypto gen-key --type ed25519
vitte crypto sign data.txt --key priv.pem
vitte crypto verify data.txt --sig data.sig --key pub.pem
```

---

## Sécurité

- Aucune clé ou donnée sensible n’est stockée sur disque sans chiffrement  
- Effacement sécurisé en mémoire via `zeroize`  
- Tests d’intégrité intégrés (self-checks) au démarrage  
- RNG basé sur `/dev/urandom`, `getrandom`, ou équivalent natif  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
rand_core = "0.6"
rand_chacha = "0.3"
sha2 = "0.10"
sha3 = "0.10"
blake3 = "1"
aes-gcm = "0.10"
chacha20poly1305 = "0.10"
ed25519-dalek = "2"
rsa = { version = "0.9", features = ["sha2"] }
hmac = "0.12"
pbkdf2 = "0.12"
argon2 = "0.5"
zeroize = "1"
base64 = "0.22"
```

---

## Tests

```bash
cargo test -p vitte-crypto
vitte crypto hash --algo blake3
```

- Tests de conformité vectorielle (RFC, NIST, Wycheproof)  
- Tests de dérivation et de vérification de signatures  
- Benchmarks de performance des algorithmes  

---

## Roadmap

- [ ] Support PQC (Dilithium, Kyber, Falcon)  
- [ ] Intégration FIPS 140-3 optionnelle  
- [ ] Génération de certificats X.509 minimalistes  
- [ ] API WebCrypto pour WASM  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau