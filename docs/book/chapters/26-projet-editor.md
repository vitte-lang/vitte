# 26. Projet guide Editeur

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Ecrire un coeur d'editeur Vitte robuste en verrouillant les invariants de curseur et l'application des commandes.

Etape 1. Definir curseur et bornes de document.

```vit
form Cursor {
  row: int
  col: int
}

form Bounds {
  max_row: int
  max_col: int
}
```

Pourquoi cette etape est solide. Les coordonnees et leurs limites sont separees pour garder des signatures de commande nettes.

Ce qui se passe a l'execution. Toute navigation sera exprimee avec `(Cursor, Bounds)`.

Etape 2. Centraliser la normalisation du curseur.

```vit
proc clamp_cursor(c: Cursor, b: Bounds) -> Cursor {
  let r: int = c.row
  let k: int = c.col
  if r < 0 { set r = 0 }
  if k < 0 { set k = 0 }
  if r > b.max_row { set r = b.max_row }
  if k > b.max_col { set k = b.max_col }
  give Cursor(r, k)
}
```

Pourquoi cette etape est solide. Les invariants de borne sont imposes en un point unique. Toute commande herite de cette garantie.

Ce qui se passe a l'execution. `clamp_cursor(Cursor(-1,99), Bounds(10,40)) -> Cursor(0,40)`.

Etape 3. Implementer des deplacements atomiques.

```vit
proc move_right(c: Cursor, b: Bounds) -> Cursor {
  let next: Cursor = Cursor(c.row, c.col + 1)
  give clamp_cursor(next, b)
}

proc move_left(c: Cursor, b: Bounds) -> Cursor {
  let next: Cursor = Cursor(c.row, c.col - 1)
  give clamp_cursor(next, b)
}
```

Pourquoi cette etape est solide. Chaque commande est une transformation locale suivie d'une normalisation deterministe.

Ce qui se passe a l'execution. A la borne droite, `move_right` ne depasse pas. A gauche de zero, `move_left` reste a zero.

Etape 4. Router des commandes typees.

```vit
pick Command {
  case Left
  case Right
}

proc apply(c: Cursor, b: Bounds, cmd: Command) -> Cursor {
  match cmd {
    case Left { give move_left(c, b) }
    case Right { give move_right(c, b) }
    otherwise { give c }
  }
}
```

Pourquoi cette etape est solide. Le dispatch est exhaustif sur une enumeration de commandes, ce qui simplifie extension et tests.

Ce qui se passe a l'execution. `apply(Cursor(2,0), b, Left)` retourne `Cursor(2,0)`.

Etape 5. Rejouer une sequence de navigation.

```vit
proc replay(c0: Cursor, b: Bounds) -> Cursor {
  let c1: Cursor = apply(c0, b, Right)
  let c2: Cursor = apply(c1, b, Right)
  let c3: Cursor = apply(c2, b, Left)
  give c3
}
```

Pourquoi cette etape est solide. La sequence composee prouve que chaque commande reste atomique et que l'invariant de borne survit au chaining.

Ce qui se passe a l'execution. Depuis `Cursor(0,0)`, la sequence `Right, Right, Left` aboutit a `Cursor(0,1)`.

Ce que vous devez maitriser en sortie de chapitre. Les commandes sont atomiques, les bornes sont centralisees et le replay reste deterministic.
