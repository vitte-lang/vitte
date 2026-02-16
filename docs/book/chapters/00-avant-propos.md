# 0. Avant-propos

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Poser la methode de travail de ce livre Vitte pour que chaque chapitre soit executable, verifiable et reutilisable en projet reel.

Etape 1. Lire Vitte comme un langage de contrats explicites.

```vit
proc contract_demo(x: int) -> int {
  if x < 0 { give 0 }
  give x
}
```

Pourquoi cette etape est solide. Une signature claire et des gardes visibles reduisent les interpretations implicites. Le style du livre suit cette discipline partout.

Ce qui se passe a l'execution. `contract_demo(-3)=0` et `contract_demo(8)=8`.

Etape 2. Travailler en boucle courte ecriture, verification, correction.

```vit
proc loop_demo(n: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i >= n { break }
    set acc = acc + i
    set i = i + 1
  }
  give acc
}
```

Pourquoi cette etape est solide. Le code est ecrit pour etre teste rapidement. La borne de boucle explicite facilite audit et debug.

Ce qui se passe a l'execution. `loop_demo(4)=6`.

Etape 3. Garder une separation stricte entre metier et projection systeme.

```vit
pick OpResult {
  case Ok(value: int)
  case Err(code: int)
}

proc to_exit(r: OpResult) -> int {
  match r {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}
```

Pourquoi cette etape est solide. Le metier produit des valeurs typees. La projection systeme est une etape finale et localisee.

Ce qui se passe a l'execution. `to_exit(Ok(42))=0` et `to_exit(Err(64))=64`.

Ce que vous devez maitriser en sortie de chapitre. Vous lisez ce livre comme un atelier technique, chaque exemple est runnable et chaque chapitre prolonge les invariants du precedent.
