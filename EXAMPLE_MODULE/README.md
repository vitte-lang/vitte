# vitte_example

Module template démontrant la structure d'un module Vitte complet.

## Description

Ce module fournit des exemples de :
- Définition de types (`ExampleStruct`, `ExampleEnum`)
- Traits personnalisés (`Processable`)
- Implémentations génériques (`Container<T>`)
- Suite de tests unitaires
- Documentation claire

## Utilisation

```vitte
use vitte_example::{ExampleStruct, ExampleEnum, Processable};

fn main() {
    // Créer une instance
    let obj = ExampleStruct::new(42, "hello".to_string());
    
    // Utiliser le trait
    println!("{}", obj.process());
    
    // Utiliser l'énumération
    let variant = ExampleEnum::WithData("world".to_string());
    println!("{}", variant.process());
}
```

## Architecture

```
src/
├── lib.vit        # Point d'entrée et réexports
├── types/         # Définitions de types et traits
│   └── lib.vit
├── impls/         # Implémentations
│   └── lib.vit
└── tests/         # Tests unitaires
    └── lib.vit
```

## Modules

### types/lib.vit
Définit les types, énumérations et traits principaux :
- `ExampleStruct`: Type structuré basique
- `ExampleEnum`: Énumération avec variantes
- `Processable`: Trait générique
- `Container<T>`: Type générique

### impls/lib.vit
Implémente les traits et fournit des méthodes :
- Constructeurs (`new`, `default`)
- Implémentations de traits (`Processable`)
- Méthodes utilitaires
- Fonctions globales

### tests/lib.vit
Suite complète de tests unitaires couvrant :
- Création et manipulation d'instances
- Comportement des variantes
- Utilisation des traits
- Types génériques

## Exécution des Tests

```bash
# Lancer tous les tests du module
cargo test --package vitte_example

# Lancer un test spécifique
cargo test --package vitte_example test_example_struct_creation

# Lancer avec affichage détaillé
cargo test --package vitte_example -- --nocapture
```

## Conventions Respectées

- ✅ Noms de types en `PascalCase`
- ✅ Noms de fonctions en `snake_case`
- ✅ Documentation avec `///` commentaires
- ✅ Tests avec attribut `#[test]`
- ✅ Organisation logique des modules
- ✅ Réexports publics dans `lib.vit`

## Extension

Pour adapter ce template à un vrai module :

1. Remplacer `ExampleStruct` par votre type
2. Adapter `ExampleEnum` à vos besoins
3. Créer des traits spécifiques au domaine
4. Ajouter les dépendances nécessaires
5. Importer depuis d'autres modules Vitte si besoin
6. Écrire des tests compréhensifs

## Voir Aussi

- [MODULE_TEMPLATE.md](../MODULE_TEMPLATE.md) - Guide complet de création de modules
- [Architecture Vitte](../STRUCTURE.md)
- [Domaines](../docs/) - Documentation des domaines
