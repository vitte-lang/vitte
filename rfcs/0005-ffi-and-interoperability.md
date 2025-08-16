# RFC 0005 — FFI and Interoperability

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0005  

---

## Résumé
Décrit le **Foreign Function Interface** de Vitte.  

## Motivation
Interopérabilité avec C, Rust, systèmes existants.  

## Détails
- Support `extern(c)`  
- Types C-compatibles : `i32`, `u64`, pointeurs.  

```vitte
extern(c) fn puts(s: *const u8) -> i32;

fn main() {
    let msg = "Hello\0";
    puts(msg.as_ptr());
}
```

- ABI par défaut = C ABI  
- FFI marquée `unsafe`  

## Alternatives
Pas d’FFI (impossible, rejeté).  

## Impact
Indispensable pour OS, libs systèmes.  

## Adoption
Implémentation progressive, vérif des signatures.  

## Références
- Rust FFI Guide  
- LLVM ABI  
- C standard ABI  
