# Stdlib (sélection)

<<<
stdlib.md
Modules stdlib sélectionnés
>>>

## Module `process`

Exécuter une commande et capturer la sortie.

Notes :
- `out` capture `stdout` et `err` capture `stderr`.

```vitte
use std/bridge/process

entry main at core/app {
  let res = run_shell("echo hello")
  when res is Result.Ok {
    let out = res.value.out
    let err = res.value.err
  }
  return 0
}
```

## Module `bridge`

Façade de liaison native avec noms Vitte.
Sous-modules :
- `std/bridge/io`
- `std/bridge/memory`
- `std/bridge/system`
- `std/bridge/time`
- `std/bridge/net`

Modules métier directs :
- `std/bridge/print`
- `std/bridge/read`
- `std/bridge/path`
- `std/bridge/env`
- `std/bridge/bytes`
- `std/bridge/alloc`
- `std/bridge/process`

```vitte
use std/bridge/system
use std/bridge/io

entry main at core/app {
  let home = get("HOME")   # std/env via bridge/system
  let p = cwd()            # std/path via bridge/system
  let _ = print_line("ok") # std/io/print via bridge/io
  return 0
}
```

## Module `system` (via bridge)

Assistants de plateforme.

```vitte
use std/bridge/system

entry main at core/app {
  let p = platform()    // "linux", "macos", "windows", "freebsd", "unknown"
  let home = home_dir() // Option[string]
  return 0
}
```

## Module `regex`

Compilation et correspondance regex.

```vitte
use std/regex

entry main at core/app {
  let re = compile("h.llo")
  when re is Result.Ok {
    let ok = is_match(re.value, "hello")
    let m = find(re.value, "hello")
  }
  return 0
}
```

## Module `fswatch`

Surveillance simple de fichiers (polling, dernière heure d'écriture).

```vitte
use std/fswatch

entry main at core/app {
  let w = watch("config.toml")
  when w is Result.Ok {
    let ev = poll(&w.value)
  }
  return 0
}
```

## Module `metrics`

Compteurs, jauges et minuteurs en mémoire.

```vitte
use std/metrics

entry main at core/app {
  let c = counter("requests")
  inc(&c, 1)
  let g = gauge("load")
  set_value(&g, 0.75)
  let t = timer("latency_ms")
  record_ms(&t, 12)
  return 0
}
```

## Module `async`

Aides pour tâches synchrones (`spawn` s'exécute immédiatement pour l'instant).

```vitte
use std/async
use std/core/types.i32

proc one() -> i32 {
  give 1
}

entry main at core/app {
  let task = spawn(one)
  let _ = poll(&task)
  return 0
}
```

## Module `db`

Base clé/valeur simple (adossée à un fichier).

```vitte
use std/db

entry main at core/app {
  let db = open("db.kv")
  when db is Result.Ok {
    let _ = begin(&db.value)
    let _ = put(&db.value, "hello", "world")
    let _ = commit(&db.value)
    let ns = namespace(&db.value, "app")
    let _ = put(ns, "key", "value")
  }
  return 0
}
```
