# RFC 0006 — Async/Await & Concurrency

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0006  

---

## Résumé
Introduit le modèle de concurrence basé sur `async`/`await` et les primitives de parallélisme de Vitte.

## Motivation
Les applications modernes nécessitent I/O asynchrone, parallélisme CPU et tâches concurrentes. Vitte doit fournir un modèle simple mais sûr.

## Détails
### Async/Await
```vitte
async fn fetch_data(url: string) -> string {
    let resp = await http_get(url);
    return resp.body;
}
```
- `async fn` crée une coroutine.  
- `await` suspend jusqu’à résultat.  

### Tâches
```vitte
let handle = spawn async {
    do_work().await;
};
handle.join();
```

### Channels
```vitte
let (tx, rx) = channel<i32>();
spawn async { tx.send(42); };
let v = rx.recv().await;
```

## Alternatives
Threads système uniquement (trop lourds).  
Callbacks (illisible).  

## Impact
- API stdlib doit inclure exécuteur (event loop).  
- Impact sur bytecode : opcodes `Await`, `Spawn`.  

## Adoption
Adopté progressivement avec stdlib async.  

## Références
- Rust async/await  
- Go goroutines  
- Erlang processes  
