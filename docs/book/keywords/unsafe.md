# Mot-cle `unsafe`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`unsafe` ouvre une zone ou les garanties standard sont reduites. Tout bloc `unsafe` doit etre encadre.

Forme de base en Vitte. `unsafe { ... }`.

Exemple 1, construit pas a pas.

```vit
proc nop_cpu() {
  unsafe { asm("nop") }
}
```

Pourquoi cette etape est solide. Zone minimale et encapsulee dans une procedure unique.

Ce qui se passe a l'execution. L'appel execute `nop` sans effet fonctionnel visible.

Exemple 2, construit pas a pas.

```vit
proc write_hw(addr: int, value: int) -> int {
  if addr < 0 { give -1 }
  unsafe {
    asm("nop")
  }
  give 0
}
```

Pourquoi cette etape est solide. Garde explicite avant entree en zone sensible.

Ce qui se passe a l'execution. `write_hw(-1,9)=-1`, `write_hw(10,9)=0`.

Exemple 3, construit pas a pas.

```vit
proc halt_cpu_if(flag: bool) -> int {
  if not flag { give 0 }
  unsafe { asm("hlt") }
  give 1
}
```

Pourquoi cette etape est solide. La condition metier controle strictement l'activation d'une instruction critique.

Ce qui se passe a l'execution. `halt_cpu_if(false)=0`. `halt_cpu_if(true)` entre en `hlt`.

Erreur frequente et correction Vitte. Etendre `unsafe` a des blocs metier entiers.

Correction recommandee en Vitte. Isoler `unsafe` dans des procedures courtes, placer les gardes avant, et documenter les preconditions.

Pour prolonger la logique. Voir `docs/book/chapters/12-pointeurs.md`, `docs/book/chapters/16-interop.md`, `docs/book/keywords/asm.md`.
