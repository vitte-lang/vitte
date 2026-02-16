# 23. Projet guide Systeme

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Construire un noyau systeme Vitte avec une discipline stricte sur les bornes memoire et l'usage de `unsafe`.

Etape 1. Declarer region memoire et codes de resultat.

```vit
form Region {
  base: int
  size: int
}

pick SysResult {
  case Ok
  case ErrBounds
  case ErrState
}
```

Pourquoi cette etape est solide. Les invariants d'adresse et les issues possibles sont fixes des le debut du projet.

Ce qui se passe a l'execution. Toute operation systeme retournera `Ok`, `ErrBounds` ou `ErrState`.

Etape 2. Centraliser la verification de bornes.

```vit
proc contains(r: Region, addr: int) -> bool {
  if addr < r.base { give false }
  if addr >= r.base + r.size { give false }
  give true
}
```

Pourquoi cette etape est solide. Cette fonction devient la precondition canonique de lecture et ecriture.

Ce qui se passe a l'execution. `contains(Region(100,16),108)=true` et `contains(...,116)=false`.

Etape 3. Isoler les instructions machine.

```vit
proc cpu_pause() {
  unsafe { asm("pause") }
}

proc cpu_halt_if(flag: bool) -> SysResult {
  if not flag { give ErrState }
  unsafe { asm("hlt") }
  give Ok
}
```

Pourquoi cette etape est solide. `pause` est non bloquant, `hlt` exige un etat prealable explicite. Les deux frontieres sont courtes et auditables.

Ce qui se passe a l'execution. `cpu_halt_if(false)=ErrState`. `cpu_halt_if(true)` entre dans l'instruction privilegiee.

Etape 4. Ecrire avec garde avant `unsafe`.

```vit
proc safe_write(r: Region, addr: int, value: int) -> SysResult {
  if not contains(r, addr) { give ErrBounds }
  unsafe {
    asm("nop")
  }
  give Ok
}
```

Pourquoi cette etape est solide. La validation precede toujours la zone non prouvable. C'est la regle structurante du code systeme.

Ce qui se passe a l'execution. Adresse hors region retourne `ErrBounds` sans entree en `unsafe`.

Etape 5. Composer ecriture et synchronisation.

```vit
proc write_then_pause(r: Region, addr: int, v: int) -> SysResult {
  let w: SysResult = safe_write(r, addr, v)
  match w {
    case Ok {
      cpu_pause()
      give Ok
    }
    case ErrBounds { give ErrBounds }
    otherwise { give ErrState }
  }
}
```

Pourquoi cette etape est solide. La composition garde un flux etat -> action -> etat sans ambiguite de transition.

Ce qui se passe a l'execution. Sur succes, sequence `safe_write` puis `cpu_pause`. Sur faute de borne, sortie immediate `ErrBounds`.

Ce que vous devez maitriser en sortie de chapitre. Les bornes sont centralisees, `unsafe` est minimal et les transitions systeme sont totalement typees.
