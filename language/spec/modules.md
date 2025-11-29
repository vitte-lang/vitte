# Modules et manifestes Muffin

## Unité de compilation
Chaque fichier `.muf` représente un manifeste Muffin et sert d’unité de
compilation. Il décrit :
* le chemin des sources `.vitte`,
* la version ou la cible,
* la liste des dépendances,
* les entrées exportées.

Exemple minimal :

```muf
muffin "std/io.vitte" {
    version: "0.2"
    entry: io
    depends: ["std/core", "runtime/buf"]
}
```

## Déclarations de module

```
module std.io:
    export only (read, write)
```

Les modules peuvent être aliasés (`module foo as bar:`) et imbriqués (`foo.bar`).

## Import / Export
* `import runtime.fs as fs`
* `export from runtime.fs`
* `export only (task_scheduler, probe_io)`

Les directives d’import utilisent la syntaxe fluide inspirée de Python, mais
restent strictes sur les newlines pour conserver la lisibilité.
