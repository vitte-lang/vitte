# RFC 0017 — Embedded Systems

- Auteur : Vincent  
- Statut : Draft  
- Date : 2025-08-16  
- Numéro : 0017  

---

## Résumé
Adapte Vitte pour systèmes embarqués (bare-metal, microcontrôleurs).

## Motivation
Vitte doit être utilisable sur Arduino, ARM Cortex, RISC-V.

## Détails
- Compilation sans stdlib (`#![no_std]`).  
- Accès direct registres mémoire.  
- Interrupt handlers.  

### Exemple
```vitte
#[no_std]
fn main() {
    let led = 0x40021018 as *mut u32;
    unsafe { *led = 1; }
}
```

## Alternatives
Support via C seulement (rejeté).  

## Impact
Large adoption IoT.  

## Adoption
Toolchain cross-compilation.  

## Références
- Rust Embedded WG  
- C bare-metal programming  
