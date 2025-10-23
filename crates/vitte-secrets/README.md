

# vitte-secrets

Système **de gestion sécurisée des secrets et identifiants** pour l’écosystème Vitte.  
`vitte-secrets` permet de stocker, chiffrer et manipuler de manière sûre les clés, tokens, certificats et mots de passe utilisés par les outils et modules Vitte.

---

## Objectifs

- Offrir une **API unifiée et sécurisée** pour la gestion des secrets.  
- Supporter le chiffrement symétrique/asymétrique et le stockage chiffré local.  
- Intégration directe avec `vitte-key`, `vitte-policy`, `vitte-cli` et `vitte-registry`.  
- Gérer la rotation, l’expiration et la validation des secrets.  
- Compatibilité avec les backends externes (`vault`, `aws-kms`, `gcp-secrets`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `vault`       | Stockage chiffré local et distant |
| `crypto`      | Chiffrement AES/GCM, RSA, Ed25519 |
| `policy`      | Gestion des droits d’accès et des rotations |
| `provider`    | Intégration avec KMS externes et gestion multi-backend |
| `keyring`     | Stockage système (macOS Keychain, GNOME Keyring, Windows Vault) |
| `audit`       | Suivi et journalisation des accès aux secrets |
| `tests`       | Vérification de cohérence et sécurité |

---

## Exemple d’utilisation

```rust
use vitte_secrets::Secrets;

fn main() -> anyhow::Result<()> {
    let mut vault = Secrets::open("~/.vitte/secrets.vault")?;
    vault.set("API_TOKEN", "1234-5678-ABCD")?;
    let token = vault.get("API_TOKEN")?;
    println!("Clé récupérée: {}", token);
    Ok(())
}
```

---

## Intégration

- `vitte-key` : génération et rotation des clés cryptographiques.  
- `vitte-policy` : contrôle d’accès aux secrets par rôles et règles.  
- `vitte-cli` : commandes `vitte secrets list`, `add`, `remove`, `rotate`.  
- `vitte-registry` : authentification sécurisée lors des publications.  
- `vitte-build` : injection contrôlée de secrets dans les environnements CI/CD.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-key = { path = "../vitte-key", version = "0.1.0" }

ring = "0.17"
base64 = "0.22"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `aes-gcm` pour chiffrement matériel, `age` pour format portable, `dirs` pour gestion du stockage local.

---

## Tests

```bash
cargo test -p vitte-secrets
```

- Tests de chiffrement/déchiffrement symétrique et asymétrique.  
- Tests de rotation et expiration automatique.  
- Tests de compatibilité multi-OS (macOS/Linux/Windows).  
- Tests de performance pour grands volumes de secrets.

---

## Roadmap

- [ ] Support complet des backends distants (Vault, AWS KMS, GCP).  
- [ ] Audit complet et alertes de sécurité.  
- [ ] Mode “hardware-bound” via TPM ou Secure Enclave.  
- [ ] Chiffrement en mémoire et effacement sécurisé.  
- [ ] Intégration visuelle dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau