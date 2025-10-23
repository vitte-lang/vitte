

# vitte-php

Backend **PHP / Zend Engine** pour le langage Vitte.  
`vitte-php` permet la compilation, l’interopérabilité et l’exécution du code Vitte dans les environnements PHP modernes, avec génération automatique de bindings et modules dynamiques.

---

## Objectifs

- Offrir un **ciblage PHP natif** pour Vitte, compatible avec PHP 8+.  
- Générer automatiquement des extensions et modules PHP depuis le code Vitte.  
- Supporter la conversion bidirectionnelle des types entre Vitte et PHP.  
- Fournir un runtime léger pour exécuter du code Vitte embarqué dans des projets PHP.  
- Intégration complète avec `vitte-codegen`, `vitte-runtime` et `vitte-ffi`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération du code PHP et des fichiers `.php`/`.so` |
| `ffi`         | Pont entre le runtime Vitte et Zend Engine via FFI |
| `convert`     | Conversion des types scalaires et objets Vitte ↔ PHP |
| `runtime`     | Gestion du cycle d’exécution Vitte dans PHP |
| `module`      | Création automatique de modules d’extension PHP |
| `analyze`     | Vérification de compatibilité des signatures |
| `tests`       | Tests d’intégration PHP/Vitte et benchmarks |

---

## Exemple d’utilisation

### Compilation d’un module PHP

```bash
vitte build --target php --out mymodule.so
```

### Appel depuis PHP

```php
<?php
include 'mymodule.php';

echo vitte_add(2, 3); // 5
?>
```

---

## Exemple de code généré

```php
function vitte_add($a, $b) {
    return $a + $b;
}
```

---

## Intégration

- `vitte-codegen` : génération du code C/PHP pour les bindings.  
- `vitte-runtime` : gestion du contexte d’exécution partagé.  
- `vitte-ffi` : communication entre le runtime Vitte et Zend Engine.  
- `vitte-docgen` : génération automatique de documentation PHPDoc.  
- `vitte-cli` : commande `vitte build --target php`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
cc = "1.0"
libffi = "3.2"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `bindgen` pour génération automatique de signatures C, `php-sys` pour interaction directe avec Zend Engine.

---

## Tests

```bash
cargo test -p vitte-php
```

- Tests de génération de modules PHP.  
- Tests d’exécution via `php -d extension=mymodule.so`.  
- Tests de compatibilité des types et fonctions.  
- Benchmarks de performance entre Vitte et PHP natif.

---

## Roadmap

- [ ] Support complet des classes et namespaces PHP.  
- [ ] Génération automatique des extensions PECL.  
- [ ] Intégration avec Composer et autoloading.  
- [ ] Mode hybride Vitte↔PHP en mémoire (sans `.so`).  
- [ ] Profilage et instrumentation via `vitte-metrics`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau