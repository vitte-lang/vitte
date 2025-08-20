# RFC 0006 — Async/Await Concurrency

## Résumé
Ce RFC définit le modèle de concurrence asynchrone du langage **Vitte**, basé sur les mots-clés `async` et `await`.  
L’objectif est de fournir un mécanisme ergonomique, performant et sûr pour la programmation concurrente, adapté aussi bien aux systèmes embarqués qu’aux applications serveur à grande échelle.

## Motivation
La concurrence est un pilier des langages modernes. Vitte doit :  
- offrir un modèle simple pour écrire du code non bloquant,  
- permettre la scalabilité des applications serveur,  
- être compatible avec l’ownership et la sûreté mémoire,  
- minimiser les surcoûts runtime (pas de GC obligatoire).  

## Design proposé

### Fonctions `async`
Une fonction déclarée `async` retourne automatiquement un `Future<T>`.  

```vitte
async fn fetch_data(url: String) -> Result<String, NetError> {
    let conn = await connect(url)
    let data = await conn.read_all()
    Ok(data)
}
```

### L’opérateur `await`
- Suspend l’exécution jusqu’à ce que le `Future` soit résolu.  
- Peut être utilisé uniquement dans un contexte `async`.  

```vitte
async fn main() {
    let page = await fetch_data("https://example.com")
    print(page)
}
```

### Exécution des tâches
- Les `Future` ne s’exécutent pas automatiquement : ils nécessitent un **runtime**.  
- La stdlib fournit un **executor léger** basé sur un poll loop.  

```vitte
fn main() {
    runtime::block_on(main_async())
}

async fn main_async() {
    let result = await fetch_data("https://vitte.dev")
    print(result)
}
```

### Tâches concurrentes
`spawn` permet de lancer des tâches concurrentes légères.  

```vitte
async fn worker(id: i32) {
    print("Worker", id)
}

fn main() {
    runtime::spawn(worker(1))
    runtime::spawn(worker(2))
    runtime::block_on(runtime::run_all())
}
```

### Communication entre tâches
- Canaux (`Channel<T>`) intégrés dans la stdlib.  
- Sécurisés par ownership et emprunts.  

```vitte
async fn producer(tx: ChannelSender<i32>) {
    for i in 0..5 {
        await tx.send(i)
    }
}

async fn consumer(rx: ChannelReceiver<i32>) {
    while let Some(v) = await rx.recv() {
        print("Reçu:", v)
    }
}
```

### Cancellation
- Futures supportent une annulation explicite (`cancel()`).  
- Utilisation sécurisée via `Drop` pour libérer les ressources.  

### Timeout
- La stdlib fournit `timeout(duration, future)`.  

```vitte
let res = await timeout(5s, fetch_data("http://slow.api"))
```

## Alternatives considérées
- **Threads natifs uniquement (C/Java)** : rejetés pour leur coût mémoire et faible scalabilité.  
- **Callbacks (Node.js, C)** : rejetés pour leur manque de lisibilité (callback hell).  
- **Green threads implicites (Go)** : rejetés pour manque de contrôle précis sur la mémoire et le scheduling.  

## Impact et compatibilité
- Impact positif : facilite la programmation réseau et concurrente.  
- Compatible avec l’ownership et la sûreté mémoire.  
- Introduit un runtime optionnel mais léger, portable sur systèmes embarqués et serveurs.  
- Prépare l’intégration avec **WASM** et environnements multi-thread.  

## Références
- [Rust Async/Await](https://rust-lang.github.io/async-book/)  
- [C# Async](https://learn.microsoft.com/en-us/dotnet/csharp/programming-guide/concepts/async/)  
- [Go Concurrency](https://go.dev/doc/effective_go#concurrency)  
