# RFC 0022 — Embedded and Systems Programming Support

## Résumé
Ce RFC définit le support de la **programmation système** et **embarqué** dans le langage **Vitte**.  
L’objectif est de permettre l’utilisation de Vitte dans des environnements contraints (microcontrôleurs, OS, kernels) tout en conservant sûreté, performance et portabilité.

## Motivation
Un langage moderne doit pouvoir :  
- cibler des systèmes embarqués à ressources limitées,  
- offrir un contrôle bas niveau (pointeurs, registres, mémoire),  
- permettre l’écriture d’OS, drivers et firmware,  
- garder la sûreté mémoire et éviter les UB (Undefined Behavior).  

## Design proposé

### Mode `no_std`
- Compilation sans bibliothèque standard :  
```toml
[package]
name = "firmware"
no_std = true
```

- Accès uniquement aux primitives du langage et `core`.  
- Pas de heap par défaut, uniquement pile.  

### Gestion mémoire
- `Box`, `Rc`, `Arc` optionnels (activés via `alloc`).  
- Contrôle explicite sur l’allocation.  
- Pointeurs bruts disponibles (`*mut T`, `*const T`) avec bloc `unsafe`.  

### Fonctions `unsafe`
- Permettent d’interagir directement avec le matériel.  
- Exemple :  
```vitte
unsafe fn write_reg(addr: *mut u32, val: u32) {
    *addr = val
}
```

### Accès registres et périphériques
- Module `std::arch` et `std::hw`.  
- Macros pour déclarer des **MMIO registers**.  
```vitte
register!(GPIO_CTRL, 0x4002_0000, u32)
fn main() {
    GPIO_CTRL.write(0x1)
}
```

### Interruptions
- Attribut `@interrupt` pour marquer une ISR.  
```vitte
@interrupt
fn timer_isr() {
    // handler code
}
```

### Concurrence bare-metal
- `Channel<T>` utilisable sans OS via spinlocks.  
- Support des coopératives schedulers (`async no_std`).  

### Toolchain embarqué
- Cibles définies dans `vitpm target add thumbv7em-none-eabihf`.  
- Génération de `.elf`, `.bin`, `.hex`.  
- Intégration avec `openocd` et `gdb`.  

### Exemple complet : LED blink
```vitte
#![no_std]
#![no_main]

use core::delay

register!(GPIO_CTRL, 0x4002_0000, u32)

fn main() {
    loop {
        GPIO_CTRL.write(0x1)
        delay::ms(500)
        GPIO_CTRL.write(0x0)
        delay::ms(500)
    }
}
```

### Support OS et kernels
- Accès direct au bootloader.  
- Possibilité d’écrire un micro-noyau en Vitte.  
- Gestion fine des segments mémoire (`.text`, `.data`, `.bss`).  

## Alternatives considérées
- **Support uniquement userland** : rejeté pour limiter Vitte.  
- **Libs externes uniquement** : rejeté pour uniformité.  
- **Pas de mode `no_std`** : rejeté, nécessaire en embarqué.  

## Impact et compatibilité
- Impact fort : ouvre Vitte aux systèmes embarqués et bas niveau.  
- Compatible avec LLVM et Cranelift pour cibles exotiques.  
- Introduit `unsafe`, mais contenu dans un modèle sûr.  

## Références
- [Rust Embedded](https://docs.rust-embedded.org/)  
- [ARM Cortex-M Architecture](https://developer.arm.com/)  
- [Zephyr RTOS](https://zephyrproject.org/)  
- [Bare-metal programming in C and Rust]  
