

# vitte-ruby

Backend **Ruby / Gems** pour le langage Vitte.  
`vitte-ruby` permet la génération de gems Ruby depuis du code Vitte et l’interopérabilité directe entre le runtime Vitte et l’interpréteur Ruby (MRI, YJIT, TruffleRuby).

---

## Objectifs

- Offrir une **intégration complète entre Vitte et Ruby**.  
- Générer automatiquement des extensions Ruby (`.so` / `.bundle`) à partir du code Vitte.  
- Supporter les appels bidirectionnels Vitte ↔ Ruby.  
- Permettre la conversion des structures (Array, Hash, String, Class).  
- Intégration directe avec `vitte-codegen`, `vitte-runtime` et `vitte-ffi`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération des wrappers Ruby C API |
| `ffi`         | Interface avec `libruby` et gestion des objets Ruby |
| `convert`     | Conversion des types Ruby ↔ Vitte |
| `runtime`     | Intégration du moteur Ruby dans l’environnement Vitte |
| `gem`         | Génération automatique des gems Ruby (`extconf.rb`, `.gemspec`) |
| `tests`       | Tests d’intégration Ruby/Vitte |

---

## Exemple d’utilisation

### Compilation d’un module Ruby

```bash
vitte build --target ruby --out vitte_math.bundle
```

### Utilisation dans Ruby

```ruby
require 'vitte_math'
puts VitteMath.add(2, 3) # => 5
```

### Exemple Vitte source

```vitte
fn add(a: int, b: int) -> int {
    return a + b
}
```

---

## Intégration

- `vitte-codegen` : production du code natif pour l’API Ruby.  
- `vitte-runtime` : exécution du code Vitte dans le contexte Ruby.  
- `vitte-ffi` : appels bas-niveau entre Vitte et Ruby C API.  
- `vitte-docgen` : génération de documentation Ruby (`.rb`, RDoc).  
- `vitte-cli` : commandes `vitte build --target ruby` et `vitte run --ruby`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

rutie = "0.9"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `rake` pour automatisation, `mkmf` pour génération de Makefile, `ffi` Ruby pour compatibilité dynamique.

---

## Tests

```bash
cargo test -p vitte-ruby
ruby tests/integration_spec.rb
```

- Tests d’import et d’exécution Ruby/Vitte.  
- Tests de conversion des types complexes.  
- Tests de compatibilité Ruby 3.0–3.3.  
- Benchmarks d’interopérabilité et de performance.

---

## Roadmap

- [ ] Génération complète de gems Ruby depuis `vitte build`.  
- [ ] Support complet des classes et modules Ruby.  
- [ ] Gestion des exceptions Ruby ↔ Vitte.  
- [ ] Support des coroutines et Fibers Ruby.  
- [ ] Visualisation des bindings dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau