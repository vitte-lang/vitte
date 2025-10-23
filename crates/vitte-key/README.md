

# vitte-key

Système **de gestion des clés, identités et signatures** pour l’écosystème Vitte.  
`vitte-key` gère la génération, le stockage et la vérification cryptographique des clés utilisées pour signer, authentifier et sécuriser les modules, builds et utilisateurs.

---

## Objectifs

- Fournir une **infrastructure de clés unifiée** pour les outils et le compilateur Vitte.  
- Supporter plusieurs algorithmes : **Ed25519**, **RSA**, **ECDSA**.  
- Gérer les clés locales, projet et globales.  
- Vérifier les signatures de modules et artefacts (`.vitpkg`, `.vitmod`).  
- Intégrer la signature automatique dans `vitte-build` et `vitte-install`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `keypair`     | Génération et gestion des paires de clés |
| `sign`        | Signature des fichiers et artefacts binaires |
| `verify`      | Vérification des signatures et certificats |
| `store`       | Gestion du stockage sécurisé (fichier ou mémoire) |
| `format`      | Encodage PEM, DER, base58 et JSON |
| `crypto`      | Abstraction sur libsodium / ring |
| `cli`         | Interface de commande (`vitte key …`) |
| `tests`       | Tests de validation et conformité cryptographique |

---

## Exemple d’utilisation

```bash
# Générer une nouvelle clé Ed25519
vitte key generate --algo ed25519 --output ~/.vitte/keys/dev.json

# Signer un binaire
vitte key sign target/release/vitte --key ~/.vitte/keys/dev.json

# Vérifier la signature
vitte key verify target/release/vitte --sig target/release/vitte.sig
```

---

## Intégration

- `vitte-build` : signature des artefacts lors des builds.  
- `vitte-install` : vérification des signatures à l’installation.  
- `vitte-cli` : gestion des identités et certificats.  
- `vitte-cache` : validation de l’intégrité des artefacts stockés.  
- `vitte-runtime` : authentification des modules externes.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

ring = "0.17"
base64 = "0.22"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
rand = "0.8"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `age` pour chiffrement de clés, `reqwest` pour récupération de certificats distants.

---

## Tests

```bash
cargo test -p vitte-key
```

- Tests de génération et validation de clés.  
- Tests de signature et vérification.  
- Tests de compatibilité entre algorithmes.  
- Tests de sécurité sur la persistance des clés.

---

## Roadmap

- [ ] Support des certificats X.509.  
- [ ] Gestion d’une autorité interne Vitte PKI.  
- [ ] Intégration GPG pour signature croisée.  
- [ ] Mode HSM (Hardware Security Module).  
- [ ] Intégration CI/CD pour signatures automatiques.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau