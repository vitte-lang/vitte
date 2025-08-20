# RFC 0005 — FFI and Interoperability

## Résumé
Ce RFC définit le modèle d’interopérabilité (FFI — Foreign Function Interface) du langage **Vitte**.  
Il spécifie comment appeler des fonctions externes (C, C++, Rust) et comment exposer du code Vitte à d’autres langages.  
L’objectif est de rendre Vitte utilisable dans des environnements hétérogènes tout en préservant sûreté et performance.

## Motivation
Un langage moderne doit pouvoir :  
- **réutiliser** les bibliothèques existantes (C, Rust, systèmes embarqués),  
- **exposer** ses fonctionnalités à d’autres langages,  
- **s’intégrer** facilement dans des systèmes existants (OS, kernels, frameworks).  

## Design proposé

### Appels externes
Un bloc `extern` déclare une fonction importée depuis une bibliothèque C.  

```vitte
extern(c) fn printf(fmt: *const u8, ...) -> i32

fn main() {
    printf("Hello %s\n".as_ptr(), "Vitte".as_ptr())
}
```

- ABI par défaut : `extern(c)` (compatibilité C).  
- Autres ABIs possibles : `extern(rust)`, `extern(vitte)`.  

### Exporter des fonctions Vitte
Le mot-clé `pub extern` permet d’exposer des fonctions Vitte vers C/Rust.  

```vitte
pub extern(c) fn somme(a: i32, b: i32) -> i32 {
    a + b
}
```

### Types FFI sûrs
- Pointeurs bruts : `*const T`, `*mut T`.  
- Types numériques compacts : `i8`, `i16`, `i32`, `i64`, `f32`, `f64`.  
- Structures avec `@repr(c)` pour compatibilité mémoire.  

```vitte
@repr(c)
struct Point {
    x: f64,
    y: f64,
}
```

### Gestion mémoire
- Le code FFI doit respecter le modèle d’ownership.  
- Les objets créés côté Vitte doivent être libérés côté Vitte.  
- Les pointeurs venant de C doivent être marqués `unsafe`.  

### Sécurité (`unsafe`)
- Les appels externes sont marqués `unsafe`.  
- Le développeur doit garantir l’absence d’UB (Undefined Behavior).  

```vitte
unsafe {
    let p = malloc(1024) // FFI C
    free(p)
}
```

### Interopérabilité avec Rust
- Support de `#[no_mangle]` et conventions d’export identiques.  
- Facilite l’intégration dans l’écosystème Rust.  

### Interopérabilité avec C++
- Utilisation d’un mode `extern(cpp)` prévu.  
- Limité aux ABI stables (sans exceptions ni templates).  

## Alternatives considérées
- **FFI implicite (comme Python ctypes)** : rejeté pour cause de lenteur et absence de typage statique.  
- **Interop exclusive C++** : rejetée pour manque de portabilité.  
- **Interop via VM seulement** : rejetée pour conserver un lien direct avec le natif.  

## Impact et compatibilité
- Impact majeur : ouvre Vitte à l’écosystème logiciel existant.  
- Compatible avec la compilation vers bytecode et natif.  
- Introduit une zone `unsafe` mais nécessaire pour les performances.  

## Références
- [Rust FFI](https://doc.rust-lang.org/nomicon/ffi.html)  
- [C ABI](https://itanium-cxx-abi.github.io/cxx-abi/)  
- [Go cgo](https://pkg.go.dev/cmd/cgo)  
