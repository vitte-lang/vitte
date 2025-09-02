# Vitte — Language Specification

## Introduction
La spécification du langage **Vitte** définit la syntaxe, la sémantique et le comportement attendu du compilateur, du runtime et de la VM.  
Elle sert de référence officielle pour les développeurs, les contributeurs et les implémenteurs tiers.

---

## Objectifs
- Définir une **syntaxe claire et minimale**.
- Spécifier des **règles sémantiques précises**.
- Garantir la **portabilité** du bytecode entre implémentations.
- Assurer la **stabilité** du langage tout en permettant l’évolution.

---

## Structure des Programmes

### Unité de compilation
Un fichier `.vit` est une unité de compilation.  
Il peut contenir :
- Déclarations (`let`, `const`, `fn`)
- Instructions (expressions, contrôle de flux)
- Imports (`import`, `from ... import`)

### Exemple minimal
```vit
fn main() {
    print("Hello, world!")
}
```

---

## Lexique

### Identifiants
- Doivent commencer par une lettre ou `_`
- Peuvent contenir lettres, chiffres, `_`
- Sensibles à la casse

Exemple : `x`, `myVar`, `_hidden`

### Mots-clés réservés
```
fn let const return if else while for break continue
import from as extern true false null
```

### Littéraux
- **Entiers** : `42`, `0xFF`, `0b1010`
- **Flottants** : `3.14`, `2e10`
- **Booléens** : `true`, `false`
- **Chaînes** : `"texte"`, `'texte'`
- **Null** : `null`

---

## Types

### Types primitifs
- `int`
- `float`
- `bool`
- `string`

### Types composites
- `array<T>`
- `map<K,V>` (prévu)
- `struct` (future extension)

### Typage
- Typage **statique** mais **inféré**
- Déclaration explicite possible :  
```vit
let x: int = 42
```

---

## Variables et Constantes

### let
Déclare une variable mutable.
```vit
let x = 10
x = 20
```

### const
Déclare une constante immuable.
```vit
const PI = 3.14159
```

---

## Fonctions

### Déclaration
```vit
fn add(x: int, y: int) -> int {
    return x + y
}
```

### Appel
```vit
let z = add(2, 3)
```

### Récursivité
```vit
fn fact(n: int) -> int {
    if n <= 1 { return 1 }
    return n * fact(n - 1)
}
```

---

## Contrôle de Flux

### if / else
```vit
if x > 0 {
    println("positif")
} else {
    println("négatif ou nul")
}
```

### while
```vit
let i = 0
while i < 5 {
    println(i)
    i = i + 1
}
```

### for (désucré en while)
```vit
for let i = 0; i < 5; i = i + 1 {
    println(i)
}
```

### break / continue
```vit
while true {
    if condition { break }
    if skip { continue }
}
```

---

## Modules et Imports

### Import simple
```vit
import math
let x = math.sqrt(16)
```

### Import sélectif
```vit
from math import sqrt, pow
```

### Alias
```vit
import system as sys
```

---

## Erreurs et Exceptions

### panic
Interrompt l’exécution immédiatement.
```vit
panic("Erreur fatale")
```

### assert
Vérifie une condition.
```vit
assert(x > 0)
```

### try / catch (roadmap)
```vit
try {
    let x = risky()
} catch e {
    println("Erreur: " + e)
}
```

---

## Bytecode (VITBC)

### Organisation
- Header (magic number, version)
- Sections :
  - `INT` : entiers
  - `FLOAT` : flottants
  - `STR` : chaînes
  - `DATA` : données binaires
  - `CODE` : instructions
- Trailer : CRC32 + options

### Exemple IR → Bytecode
```text
LOAD_CONST r1, 2
LOAD_CONST r2, 3
ADD r3, r1, r2
PRINT r3
```

---

## Interopérabilité

### FFI (Foreign Function Interface)
Permet d’appeler du code natif.
```vit
extern fn c_sqrt(x: float) -> float

let r = c_sqrt(9.0)
```

---

## Roadmap Langage
- [x] Fonctions, variables, const
- [x] if / while / for
- [x] Types primitifs
- [x] Imports de base
- [ ] Collections natives
- [ ] Exceptions structurées
- [ ] Structures et enums
- [ ] Génériques
- [ ] Concurrence (threads, async/await)

---

## Conclusion
La spécification Vitte définit les **fondations solides** du langage :  
un cœur simple, extensible et pensé pour évoluer vers un écosystème moderne.  
Elle constitue la **référence officielle** de tout développement Vitte.