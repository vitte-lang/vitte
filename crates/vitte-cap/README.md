

# vitte-cap

Infrastructure **à capacités** pour Vitte : permissions minimales, contrôle d’accès et exécution confinée.  
`vitte-cap` fournit un modèle à capacités non ambiant, des jetons signés, des politiques d’autorisation et des garde‑fous d’exécution pour le toolchain et le runtime Vitte.

---

## Objectifs

- Modèle à **capacités** explicites (pas d’ambient authority).  
- Jetons portables, vérifiables et **signés** (atténuables).  
- Portées temporelles, *scopes* hiérarchiques et délégation contrôlée.  
- Permissions composables : `fs.read`, `fs.write`, `net.connect`, `proc.spawn`, `ffi.load`, `device.audio`, `gpu.compute`, `cap.delegate`.  
- **Audit** des décisions, traçabilité et rejouabilité des vérifications.  
- API sync/async, **no_std** optionnelle côté runtime embarqué.

---

## Architecture

| Module       | Rôle |
|--------------|------|
| `model`      | Types de base : `Cap`, `Scope`, `Claim`, `Decision`, `Reason` |
| `token`      | Sérialisation des capacités, caveats, expiration, *nonce* |
| `signer`     | Issuance et signature (Ed25519) des jetons |
| `verifier`   | Vérifications cryptographiques et de caveats |
| `policy`     | Stratégies d’autorisation (RBAC par rôles → CapSet, règles dynamiques) |
| `gate`       | Garde appliquant `check(op, context)` et produisant un `Decision` auditable |
| `sandbox`    | Enforcers OS : FS restreint, réseau filtré, sous‑processus, FFI gated |
| `store`      | Stockage des clés et *trust anchors* (mémoire, fichier, HSM ultérieurement) |
| `context`    | Contexte d’exécution, identités d’acteurs, corrélations de requêtes |
| `serde`      | Encodage/decodage (JSON/CBOR) des jetons et journaux |
| `tests`      | Tests d’autorisation, de non‑régression et *property‑based*

---

## Exemple d’utilisation

```rust
use vitte_cap::{Cap, CapSet, Gate, Policy, Scope, Signer, Verifier, Decision};
use std::time::{Duration, SystemTime};

fn main() -> anyhow::Result<()> {
    // 1) Autorité émettrice
    let issuer = Signer::generate();
    let verifier = Verifier::from_public_key(issuer.public_key());

    // 2) Définir le scope et les capacités
    let build_scope = Scope::new("vitte-build")
        .with_ttl(Duration::from_secs(3600))
        .with_not_before(SystemTime::now());

    let caps = CapSet::new()
        .allow(Cap::fs_read_prefix("./src"))
        .allow(Cap::fs_write_prefix("./target"))
        .allow(Cap::proc_spawn(["cc", "ar"]))
        .allow(Cap::net_connect_host("crates.io", 443));

    // 3) Émettre un jeton signé avec caveats
    let token = issuer.issue_token(build_scope, caps)?
        .with_caveat_time_window(Duration::from_secs(600)) // atténuation
        .seal()?;

    // 4) Vérifier et décider au moment d’une opération sensible
    let policy = Policy::default();
    let gate = Gate::new(policy, verifier);

    let op = Cap::fs_write_prefix("./target");
    match gate.check(&token, &op) {
        Decision::Allow(meta) => {
            println!("ok: {:?}", meta);
            // exécuter l’opération réelle ici
        }
        Decision::Deny(reason) => {
            anyhow::bail!("refusé: {reason:?}");
        }
    }
    Ok(())
}
```

---

## Intégration

- `vitte-build` : confinement du pipeline, accès FS et spawn contrôlés.  
- `vitte-compiler` : FFI, JIT et IO protégés par capacités.  
- `vitte-lsp` : lecture projet limitée aux scopes autorisés.  
- `vitte-runtime` : exécution de modules avec *cap sets* minimaux.  
- `vitte-docgen` : lecture seule des sources et écriture dans `./site`.

---

## Dépendances

```toml
[dependencies]
bitflags = "2"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
rand = "0.8"
ed25519-dalek = { version = "2", features = ["rand_core"] }
sha2 = "0.10"
parking_lot = "0.12"
``` 

> Optionnel : `ciborium` pour CBOR, `time` pour politiques avancées, `tracing` pour audit.

---

## Tests

```bash
cargo test -p vitte-cap
```

- Tests d’autorisation (*allow/deny*) et d’atténuation.  
- Fuzzing de *caveats* et de sérialisation.  
- Tests d’intégration avec `vitte-build` (mock FS et spawn).

---

## Roadmap

- [ ] Caveats de type *macaroons* (atténuation et délégation chaînée).  
- [ ] Révocation et *short‑lived tokens* avec rotation de clés.  
- [ ] Ponts OS : macOS App Sandbox, Linux `seccomp`/`landlock`, OpenBSD `pledge`/`unveil`.  
- [ ] Profiles prédéfinis pour CI et runners distants.  
- [ ] Encodage CBOR compact et signatures agrégées.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau