# RFC 0009 — Std Library Structure

## Résumé
Ce RFC définit la structure et l’organisation de la **bibliothèque standard (stdlib)** du langage **Vitte**.  
L’objectif est de proposer une hiérarchie claire, modulaire et cohérente, couvrant les besoins fondamentaux des développeurs, tout en restant extensible.

## Motivation
La bibliothèque standard est un élément essentiel d’un langage :  
- elle détermine l’ergonomie et la productivité des développeurs,  
- elle impose des conventions et bonnes pratiques,  
- elle définit l’écosystème de base sur lequel les bibliothèques externes se construisent,  
- elle permet d’écrire du code portable sans dépendances excessives.  

## Design proposé

### Principes de conception
1. **Modularité** : séparation claire en modules spécialisés.  
2. **Minimalisme** : éviter le surchargement, ne garder que l’essentiel.  
3. **Interopérabilité** : compatible avec FFI et les environnements système.  
4. **Portabilité** : la stdlib doit fonctionner sur desktop, serveur, embarqué et WASM.  
5. **Sécurité** : intégration directe avec ownership, erreurs, et concurrence sûres.  

### Organisation des modules principaux

```
std/
 ├── core/          # primitives de base, types fondamentaux
 ├── collections/   # structures de données (Vec, Map, Set…)
 ├── string/        # chaînes de caractères, encodages
 ├── io/            # entrées/sorties (fichiers, réseau, console)
 ├── time/          # gestion du temps, dates, durées
 ├── math/          # fonctions mathématiques, algèbre, trigonométrie
 ├── net/           # TCP, UDP, HTTP (asynchrone)
 ├── sync/          # primitives de synchronisation (Mutex, RwLock)
 ├── async/         # futures, tâches, runtime async
 ├── sys/           # appels systèmes, processus, threads
 ├── fs/            # système de fichiers
 ├── error/         # gestion des erreurs communes
 └── test/          # framework de tests intégré
```

### Exemples de modules

#### `std::collections`
```vitte
let mut v = Vec::new()
v.push(1)
v.push(2)

let mut m = HashMap::new()
m.insert("clé", 42)
```

#### `std::string`
```vitte
let s = String::new("Bonjour")
let t = s + " Vitte!"
```

#### `std::io`
```vitte
use std::io::{File, Read, Write}

fn main() -> Result<(), IOError> {
    let mut f = File::open("data.txt")?
    let contenu = f.read_to_string()?
    print(contenu)
    Ok(())
}
```

#### `std::net`
```vitte
async fn client() {
    let mut sock = await TcpStream::connect("127.0.0.1:8080")
    await sock.write("ping")
    let rep = await sock.read()
    print(rep)
}
```

#### `std::test`
```vitte
#[test]
fn addition() {
    assert_eq!(2 + 2, 4)
}
```

### Extensions prévues
- `std::crypto` : primitives de cryptographie.  
- `std::simd` : calcul vectoriel pour HPC.  
- `std::ffi` : outils d’interopérabilité avancée.  
- `std::wasm` : support WebAssembly.  

## Alternatives considérées
- **Stdlib géante (Java, .NET)** : rejetée pour éviter la lourdeur et favoriser les libs externes.  
- **Stdlib minimale (C)** : rejetée pour éviter la réécriture permanente des mêmes outils.  
- **Dépendance à un écosystème externe (Go avec x/)** : rejetée pour conserver l’autonomie du langage.  

## Impact et compatibilité
- Impact direct sur l’ergonomie et l’adoption du langage.  
- Compatible avec le package manager `vitpm`.  
- Standardise les conventions de nommage et l’organisation du code.  
- Prépare la future extension vers les plateformes embarquées et WASM.  

## Références
- [Rust std](https://doc.rust-lang.org/std/)  
- [C++ STL](https://en.cppreference.com/w/cpp)  
- [Go standard library](https://pkg.go.dev/std)  
- [Java Standard API](https://docs.oracle.com/javase/8/docs/api/)  
