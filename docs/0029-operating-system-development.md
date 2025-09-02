# RFC 0029 — Operating System Development

## Résumé
Ce RFC définit le support du **développement de systèmes d’exploitation (OS)** dans le langage **Vitte**.  
Objectif : permettre aux ingénieurs systèmes d’utiliser Vitte pour créer des noyaux, pilotes, bootloaders et systèmes embarqués, avec sûreté mémoire et performance natives.

## Motivation
Un langage bas-niveau doit :  
- permettre l’accès direct au matériel,  
- gérer la mémoire sans dépendances externes,  
- supporter la compilation bare-metal (sans OS),  
- offrir des abstractions sûres pour écrire du code noyau.  

Sans cela, Vitte resterait cantonné aux applications haut niveau.

## Design proposé

### Compilation bare-metal
- `vitpm build --target x86_64-baremetal`  
- `vitpm build --target aarch64-baremetal`  
- Génération d’ISO bootables.  

### Module `std::osdev`
- Accès bas-niveau : interruptions, registres CPU, ports I/O.  
- Gestion des interruptions matérielles :  

```vitte
use std::osdev::interrupts

fn handler_irq0() {
    print("Timer interrupt")
}

fn main() {
    interrupts::register(0, handler_irq0)
    interrupts::enable()
}
```

### Gestion mémoire
- Allocation physique/virtuelle.  
- Paging, segmentation, heap custom.  

```vitte
use std::osdev::mem

fn main() {
    let page = mem::alloc_page()
    mem::map(page, 0x1000)
}
```

### Pilotes matériels
- Drivers écrits en Vitte via `std::osdev::driver`.  
- Exemple driver clavier :  

```vitte
use std::osdev::driver

fn on_key(scancode: u8) {
    print("Key pressed: {}", scancode)
}

fn main() {
    driver::keyboard::set_callback(on_key)
}
```

### Boot et noyau
- Bootloader minimal généré avec `vitboot`.  
- Exemple noyau :  

```vitte
#![no_std]
fn main() {
    print("Hello from Vitte Kernel!")
    loop {}
}
```

### Concurrence noyau
- Support SMP (multi-core).  
- Primitives `spinlock`, `semaphore`, `atomic`.  

### Exemple complet : noyau jouet
```vitte
use std::osdev::{interrupts, driver}

fn irq_timer() {
    print("Tick")
}

fn main() {
    interrupts::register(0, irq_timer)
    driver::vga::print("Booted Vitte OS")
    loop {}
}
```

### CI/CD et tests
- Exécution noyau dans QEMU :  
```sh
vitpm os test --qemu
```  
- Tests unitaires bare-metal avec mocks.  

## Alternatives considérées
- **Ne pas supporter OS dev** : rejeté, langage perdrait crédibilité systèmes.  
- **Passer uniquement par C interop** : rejeté, trop verbeux et dangereux.  
- **Support uniquement x86** : rejeté, ARM incontournable.  

## Impact et compatibilité
- Impact fort : Vitte devient un langage viable pour OS dev.  
- Compatible avec environnements bare-metal, hyperviseurs et embedded.  
- Introduit complexité bas-niveau mais essentielle pour adoption systèmes.  

## Références
- [Rust OSDev](https://os.phil-opp.com/)  
- [Linux Kernel](https://www.kernel.org/)  
- [Redox OS](https://www.redox-os.org/)  
- [seL4 Microkernel](https://sel4.systems/)  
