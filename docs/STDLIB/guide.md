# Vitte Stdlib — Guide Complet

## Introduction
La **bibliothèque standard (stdlib)** de Vitte constitue l’ensemble des fonctionnalités de base disponibles pour tous les programmes.  
Elle est conçue pour être **minimaliste, portable et extensible**, tout en couvrant les besoins essentiels (I/O, chaînes, mathématiques, collections).

---

## Objectifs
- Fournir un **socle commun** utilisable sans dépendances externes.
- Garantir une **portabilité totale** (Windows, Linux, macOS, BSD, WASM).
- Favoriser la **simplicité** et la **robustesse**.
- Servir de **base** pour construire des bibliothèques communautaires.

---

## Organisation de la Stdlib

- **core** : primitives de base (types, runtime API)
- **io** : entrées/sorties (console, fichiers)
- **math** : fonctions mathématiques
- **string** : manipulation de chaînes
- **collections** : listes, dictionnaires (roadmap)
- **time** : gestion du temps et des dates
- **system** : interaction avec l’environnement

---

## Modules en détail

### 1. core
Fonctionnalités :
- Types de base (`int`, `float`, `bool`, `string`)
- Conversions (`int_to_float`, `str_to_int`)
- Erreurs et exceptions (`panic`, `assert`)

Exemple :
```vit
let x = str_to_int("42")
assert(x == 42)
```

---

### 2. io
Fonctionnalités :
- `print`, `println`
- Lecture clavier (`read_line`)
- Fichiers : `open`, `read`, `write`, `close`

Exemple :
```vit
println("Hello, world!")
let name = read_line()
println("Salut " + name)
```

---

### 3. math
Fonctionnalités :
- Opérations classiques : `abs`, `pow`, `sqrt`
- Trigonométrie : `sin`, `cos`, `tan`
- Random : `rand`, `rand_range`

Exemple :
```vit
let x = sqrt(16.0)
println("Racine de 16 = " + str(x))
```

---

### 4. string
Fonctionnalités :
- `len`, `substr`, `split`, `replace`
- Concaténation avec `+`
- Comparaisons (`==`, `!=`, `<`, `>`)

Exemple :
```vit
let s = "Vitte"
println(len(s))       # 5
println(substr(s, 1)) # "itte"
```

---

### 5. collections (roadmap)
Fonctionnalités prévues :
- **Listes** : dynamiques, push/pop, accès par index
- **Maps** : paires clé/valeur
- **Iterators** : boucle `for` intégrée

Exemple (futur) :
```vit
let l = [1, 2, 3]
l.push(4)
for x in l {
    println(x)
}
```

---

### 6. time
Fonctionnalités :
- `now` : timestamp actuel
- `sleep(ms)` : pause
- Conversion en date lisible

Exemple :
```vit
println("Start")
sleep(1000)
println("1 seconde plus tard")
```

---

### 7. system
Fonctionnalités :
- Accès aux arguments (`args`)
- Informations système (`os_name`, `arch`)
- Variables d’environnement (`env_get`, `env_set`)

Exemple :
```vit
println(os_name())   # "Linux"
println(arch())      # "x86_64"
```

---

## Intégration avec la VM
Chaque appel stdlib est :
1. Résolu par le **Runtime API**
2. Vérifié (signature, types)
3. Exécuté dans un environnement sandboxé
4. Retourne une valeur utilisable en bytecode

---

## Exemple complet

### Code source
```vit
fn main() {
    println("Bienvenue dans Vitte!")
    let n = str_to_int(read_line())
    println("Carré = " + str(n * n))
}
```

### Exécution
```
Bienvenue dans Vitte!
> 5
Carré = 25
```

---

## Roadmap Stdlib
- [x] `core`, `io`, `math`, `string`
- [x] `time`, `system`
- [ ] `collections` avancées
- [ ] Support JSON / XML
- [ ] Réseau (`net` : sockets, HTTP)
- [ ] Multi-threading (`thread`, `channel`)
- [ ] API crypto (hash, encodage, chiffrement)

---

## Conclusion
La **stdlib Vitte** est conçue comme une boîte à outils de base, équilibrant **simplicité, portabilité et extensibilité**.  
Elle constitue un pilier essentiel de l’écosystème, permettant aux développeurs de coder efficacement dès le premier fichier.