# Template pour Créer un Nouveau Module Vitte

Guide complet pour créer de nouveaux modules dans l'architecture Vitte.

## Structure de Répertoires

```
foundation/core/vitte_example/
├── Vitte.toml              # Manifeste du module
├── src/
│   ├── lib.vit            # Point d'entrée principal
│   ├── types/
│   │   └── lib.vit        # Définitions de types
│   ├── impls/
│   │   └── lib.vit        # Implémentations
│   └── tests/
│       └── lib.vit        # Tests unitaires
└── README.md               # Documentation du module
```

## 1. Créer le Manifeste (Vitte.toml)

```toml
[package]
name = "vitte_example"
version = "0.0.0"
edition = "2024"

[dependencies]
# Dépendances internes (même domaine)
vitte_related_crate = { path = "../vitte_related_crate" }

# Dépendances externes d'autres domaines
vitte_foundation_module = { path = "../../utils/vitte_foundation_module" }

# Dépendances externes (crates publiques)
serde = { version = "1.0", features = ["derive"], optional = true }

[dev-dependencies]
# Outils de test

[features]
# Features optionnelles du module
```

## 2. Créer le Point d'Entrée (lib.vit)

```vitte
// Déclaration des sous-modules
pub mod types;
pub mod impls;

// Réexports principaux
pub use types::*;
pub use impls::*;

/// Documentation du module
/// 
/// Ce module fournit...
```

## 3. Structure des Types (types/lib.vit)

```vitte
// Types publics principaux
pub struct ExampleType {
    pub field1: String,
    pub field2: i32,
}

pub enum ExampleEnum {
    Variant1,
    Variant2(String),
}

pub trait ExampleTrait {
    fn method(&self) -> String;
}

// Types internes (non exportés)
struct InternalType {
    data: Vec<u8>,
}
```

## 4. Structure des Implémentations (impls/lib.vit)

```vitte
use crate::types::*;

impl ExampleType {
    pub fn new(field1: String, field2: i32) -> Self {
        Self { field1, field2 }
    }

    pub fn method(&self) -> String {
        format!("{}: {}", self.field1, self.field2)
    }
}

impl ExampleTrait for ExampleType {
    fn method(&self) -> String {
        self.method()
    }
}
```

## 5. Tests Unitaires (tests/lib.vit)

```vitte
#[cfg(test)]
mod tests {
    use crate::*;

    #[test]
    fn test_creation() {
        let obj = ExampleType::new("test".to_string(), 42);
        assert_eq!(obj.field2, 42);
    }

    #[test]
    fn test_method() {
        let obj = ExampleType::new("hello".to_string(), 10);
        assert!(obj.method().contains("hello"));
    }
}
```

## 6. Documentation du Module (README.md)

```markdown
# vitte_example

Brève description du module.

## Utilisation

```vitte
use vitte_example::{ExampleType, ExampleTrait};

fn main() {
    let obj = ExampleType::new("example".to_string(), 42);
    println!("{}", obj.method());
}
```

## Architecture

- **types/**: Définitions de types et traits
- **impls/**: Implémentations des types et traits
- **tests/**: Suite de tests

## Dépendances

- `vitte_related_crate`: Description
```

---

## Hiérarchie des Domaines

### Foundation Modules
**Location**: `foundation/`

```
foundation/
├── core/          # Modules noyau (erreurs, diagnostics, types de base)
└── utils/         # Modules utilitaires (structures de données, allocation)
```

**Exemples**:
- `vitte_errors`: Gestion des erreurs
- `vitte_span`: Gestion des positions de code source
- `vitte_data_structures`: Structures de données communes

### Lingua Modules
**Location**: `lingua/`

```
lingua/
├── syntax/        # Modules de syntaxe et parsing
└── types/         # Modules de type et analyse sémantique
```

**Exemples**:
- `vitte_lexer`: Tokenization
- `vitte_parse`: Parsing
- `vitte_hir`: Représentation intermédiaire haute niveau
- `vitte_type_check`: Vérification de types

### Compiler Modules
**Location**: `compiler/`

```
compiler/
├── ir/            # Représentations intermédiaires bas niveau
└── backends/      # Générateurs de code cible
```

**Exemples**:
- `vitte_mir`: Représentation intermédiaire moyenne
- `vitte_codegen_llvm`: Backend LLVM

---

## Commandes Utiles

### Créer un nouveau module dans foundation/utils:

```bash
# 1. Créer la structure
mkdir -p foundation/utils/vitte_newmodule/src/{types,impls,tests}

# 2. Copier le template Vitte.toml
cp MODULE_TEMPLATE_VITTE.toml foundation/utils/vitte_newmodule/Vitte.toml

# 3. Créer les fichiers source
touch foundation/utils/vitte_newmodule/src/lib.vit
touch foundation/utils/vitte_newmodule/src/types/lib.vit
touch foundation/utils/vitte_newmodule/src/impls/lib.vit
touch foundation/utils/vitte_newmodule/src/tests/lib.vit
touch foundation/utils/vitte_newmodule/README.md

# 4. Ajouter les dépendances dans Vitte.toml
# 5. Éditer les fichiers source
```

### Vérifier la structure d'un module:

```bash
tree foundation/utils/vitte_example -I 'target'
```

---

## Conventions de Nommage

- **Fichiers**: Utiliser `snake_case` (ex: `vitte_my_module`)
- **Types**: Utiliser `PascalCase` (ex: `MyType`)
- **Fonctions**: Utiliser `snake_case` (ex: `my_function`)
- **Constantes**: Utiliser `SCREAMING_SNAKE_CASE` (ex: `MY_CONSTANT`)
- **Modules**: Utiliser `snake_case` (ex: `mod my_submodule`)

---

## Checklist pour Nouveau Module

- [ ] Répertoire créé avec structure complète
- [ ] Vitte.toml configuré avec dépendances correctes
- [ ] lib.vit créé avec déclarations de modules
- [ ] types/lib.vit rempli avec les types principaux
- [ ] impls/lib.vit rempli avec les implémentations
- [ ] tests/lib.vit avec au moins quelques tests
- [ ] README.md documenté
- [ ] Dépendances du module parent mises à jour
- [ ] Vérification de la compilation

---

## Ressources

- [Architecture Vitte](./STRUCTURE.md)
- [Guide du Langage Vitte](./docs/)
- [Exemples d'Implémentation](./foundation/core/)
