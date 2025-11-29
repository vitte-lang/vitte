# Blocs système / kernel

Les mots-clés `interrupt`, `boot`, `map`, `port`, `reg`, `layout`, `unsafe`,
`intrinsic`, `extern`, `asm` servent à encapsuler des blocs à bas niveau.

```vitte
kernel micro:
    boot init:
        map memory:
            layout:
                reg control = 0xFF00
        unsafe:
            asm {
                mov r0, #0
            }
```

* `interrupt <nom>:` déclare un handler de bas niveau.
* `boot`: initialisation d’un kernel ou firmware.
* `map` / `layout`: description mémoire, tables de pages, MMIO.
* `unsafe` / `asm`: zones explicitement hors vérification statique.
* `intrinsic` / `extern`: ponts vers des fonctions runtime/ABI.

Ces blocs peuvent contenir des instructions classiques ou d’autres blocs système.
