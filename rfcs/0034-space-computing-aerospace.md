# RFC 0034 — Space Computing & Aerospace Applications

## Résumé
Ce RFC définit le support du **calcul spatial** et des **applications aérospatiales** dans le langage **Vitte**.  
Objectif : permettre aux ingénieurs d’utiliser Vitte pour développer des logiciels embarqués dans les satellites, fusées, sondes interplanétaires et systèmes aéronautiques, avec sûreté et performance.

## Motivation
Les logiciels spatiaux et aéronautiques nécessitent :  
- sûreté mémoire et absence d’UB (undefined behavior),  
- performance temps réel,  
- tolérance aux pannes et redondance,  
- compatibilité avec environnements contraints (RTOS, bare-metal).  

Vitte doit fournir une base robuste pour ces scénarios critiques.

## Design proposé

### Compilation cross-platform
- `vitpm build --target armv7-rtos`  
- `vitpm build --target riscv64-baremetal`  
- Support FPGA + ASIC (via LLVM backends).  

### Module `std::space`
- Outils pour contrôle embarqué spatial.  
- Exemple : gestion capteurs.  

```vitte
use std::space::sensors

fn main() {
    let temp = sensors::temperature()
    print("Temperature: {}", temp)
}
```

### Communications spatiales
- Support CCSDS, protocoles spatiaux.  
- Exemple :  

```vitte
use std::space::comms

fn main() {
    comms::send("Hello Earth")
}
```

### Tolérance aux fautes
- Types `RadSafe<T>` pour radiation safety.  
- Redondance triple modular (TMR).  

```vitte
use std::space::fault

fn main() {
    let data = fault::RadSafe::new(42)
    print(data.get())
}
```

### Temps réel
- Intégration avec RTOS (FreeRTOS, RTEMS).  
- Primitives : `task`, `deadline`, `priority`.  

```vitte
use std::rtos

fn main() {
    rtos::task(priority=1, deadline=10ms, || {
        control_thruster()
    })
}
```

### Exemple complet : satellite minimal
```vitte
use std::space::{sensors, comms, fault}

fn main() {
    let temp = sensors::temperature()
    let safe_temp = fault::RadSafe::new(temp)
    comms::send(format!("Temp: {}", safe_temp.get()))
}
```

## Alternatives considérées
- **Ne pas supporter espace** : rejeté, Vitte veut viser mission-critical.  
- **Support uniquement aéronautique** : rejeté, spatial est complémentaire.  
- **Laisser aux RTOS externes** : rejeté, manque d’intégration native.  

## Impact et compatibilité
- Impact fort : Vitte devient viable pour l’aérospatial et le spatial.  
- Compatible RTOS, bare-metal, FPGA.  
- Ajoute crédibilité pour systèmes critiques.  

## Références
- [NASA Core Flight System](https://cfs.gsfc.nasa.gov/)  
- [ESA Software Engineering Standards](https://ecss.nl/)  
- [RTEMS RTOS](https://www.rtems.org/)  
- [FreeRTOS](https://www.freertos.org/)  
