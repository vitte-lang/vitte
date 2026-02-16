# Mot-cle `as`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`as` sert a l'alias d'import, a l'annotation explicite de construction, et a certaines conversions explicites.

Forme de base en Vitte. Formes courantes: `use ... as alias`, `make x as Type = ...`, `expr as Type`.

Exemple 1, construit pas a pas.

```vit
use std/core/types.int as i32

proc inc(x: i32) -> i32 {
  give x + 1
}

entry main at core/app {
  let v: i32 = inc(41)
  return v
}
```

Pourquoi cette etape est solide. L'alias d'import n'altere pas la representation machine du type, mais il fixe une convention de lecture locale. Le compilateur remplace `i32` par la resolution de symbole correspondante dans le module importe.

Ce qui se passe a l'execution. L'appel `inc(41)` produit `42`. Le flux runtime est identique a une version sans alias.

Exemple 2, construit pas a pas.

```vit
proc cast_demo(value: int) -> int {
  make n as int = value as int
  if n < 0 { give 0 }
  give n
}
```

Pourquoi cette etape est solide. `make ... as int` fixe la forme locale de la liaison. `value as int` laisse une trace explicite de conversion au point de frontiere.

Ce qui se passe a l'execution. `cast_demo(9)=9`. `cast_demo(-3)=0`.

Exemple 3, construit pas a pas.

```vit
use app/domain/user as ud

proc same_user(a: ud.UserId, b: ud.UserId) -> bool {
  give a == b
}
```

Pourquoi cette etape est solide. `as` permet d'eviter des chemins longs repetes et rend explicite la provenance de type dans le module local.

Ce qui se passe a l'execution. `same_user(7,7)=true`. `same_user(7,8)=false`.

Erreur frequente et correction Vitte. Utiliser `as` pour masquer un mauvais design de types.

Correction recommandee en Vitte. Corriger d'abord la signature et les alias de domaine, puis garder `as` comme marqueur de frontiere, pas comme rustine.

Pour prolonger la logique. Voir `docs/book/logique/expressions.md`, `docs/book/keywords/type.md`, `docs/book/keywords/is.md`.
