# RFC 0002 — Module System

## Résumé
Ce RFC définit le système de modules du langage Vitte.  
Il décrit la manière dont le code est organisé en unités logiques réutilisables, comment les symboles sont importés/exportés, et comment les dépendances entre modules sont résolues.

## Motivation
Un langage moderne a besoin d’un système de modules :
- pour organiser le code source en composants clairs et maintenables,
- pour permettre la réutilisation et la factorisation des fonctionnalités,
- pour garantir l’isolation des symboles et éviter les conflits de noms,
- pour préparer l’intégration avec le gestionnaire de paquets `vitpm`.

## Design proposé

### Définition d’un module
Un module correspond à :
- un fichier unique (`math.vit`),
- ou un dossier avec un `mod.vit` comme point d’entrée.

```vitte
// fichier: math.vit
pub fn square(x: i32) -> i32 {
    x * x
}
```

### Hiérarchie des modules
Les dossiers définissent des sous-modules :
```
src/
 ├── mod.vit        // module racine
 ├── math/
 │    ├── mod.vit   // sous-module math
 │    ├── trig.vit  // sous-module math::trig
```

### Exportation (`pub`)
- Par défaut, tout symbole est privé au module.
- `pub` rend le symbole accessible depuis l’extérieur.

```vitte
pub struct Point { x: f64, y: f64 }
```

### Importation (`use`)
- Permet d’accéder à des symboles d’un autre module.
- Supporte les alias (`as`) et l’import multiple.

```vitte
use math::square
use math::trig::{sin, cos}
use io::fs as filesystem
```

### Packages et dépendances
- Chaque projet est un **package** racine.
- Un package peut exposer des modules publics.
- Les dépendances externes sont résolues via `vitpm`.

### Exemples
```vitte
mod geometry {
    pub fn area_square(side: f64) -> f64 {
        side * side
    }
}

use geometry::area_square

fn main() {
    let a = area_square(4.0)
    print(a)
}
```

## Alternatives considérées
- **Modules implicites par hiérarchie de dossiers (Python-like)** : rejeté pour éviter la magie implicite et garder un contrôle explicite.
- **Namespaces uniques (C++)** : rejeté car trop verbeux et peu ergonomique.
- **Système basé uniquement sur imports relatifs** : rejeté pour la clarté des dépendances globales.

## Impact et compatibilité
- Impact fort sur la structure des projets et la lisibilité.
- Compatible avec le futur gestionnaire de paquets `vitpm`.
- Impact sur la compilation incrémentale : chaque module pourra être compilé indépendamment.
- Permet une meilleure ergonomie pour les IDE et l’autocomplétion.

## Références
- [Rust Module System](https://doc.rust-lang.org/reference/items/modules.html)
- [C# Namespaces](https://learn.microsoft.com/en-us/dotnet/csharp/programming-guide/namespaces/)
- [Python Modules](https://docs.python.org/3/tutorial/modules.html)
