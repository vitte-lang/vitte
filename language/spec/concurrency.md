# Concurrence Vitte

## Primitives
* `task <id>:` définit un bloc concurrent léger.
* `spawn func()` lance une tâche et récupère un handle implicite.
* `await expr` suspend jusqu’à complétion.
* `parallel:` exécute des sous-blocs en parallèle structurée.
* `race:` évalue plusieurs branches et retient la première.
* `channel <id> = [buffer]` déclare un canal (buffer optionnel).

## Exemple

```vitte
task pipeline:
    channel queue = []

    spawn producer(queue)
    spawn consumer(queue)

    await drain(queue)
```

Les constructions sont volontairement proches du vocabulaire des langages
modernes (Rust async, Go, JS) tout en restant textuellement simples. Les canaux
peuvent être utilisés dans des expressions (`queue.push(x)`), tandis que les
blocs `parallel`/`race` fournissent une base pour des kernels réactifs.
