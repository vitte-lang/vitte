# RFC 0016 — Concurrency & Parallelism

- Auteur : Vincent  
- Statut : Draft  
- Date : 2025-08-16  
- Numéro : 0016  

---

## Résumé
Spécifie les mécanismes de concurrence (concurrent tasks) et de parallélisme (threads multiples) dans Vitte.

## Motivation
Offrir une base performante pour les applications multi-cœurs.

## Détails
- `spawn` pour lancer des tâches concurrentes.  
- Threads mappés sur OS.  
- Synchronisation : Mutex, RwLock, Channel.  

### Exemple
```vitte
let counter = Arc::new(Mutex::new(0));

for i in 0..4 {
    let c = counter.clone();
    spawn thread {
        let mut v = c.lock();
        *v += 1;
    };
}
```

## Alternatives
Pas de parallélisme intégré (limité).  

## Impact
Doit équilibrer perf et sécurité.  

## Adoption
Basé sur libstd `sync`.  

## Références
- Rust std::sync  
- Go goroutines  
- Java concurrency  
