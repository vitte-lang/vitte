# 27. Grammaire du langage

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Lire la grammaire Vitte comme un guide de construction de phrases valides pour accelerer diagnostic et ecriture.

Etape 1. Construire une declaration de procedure valide.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Pourquoi cette etape est solide. Signature complete, bloc delimite et expression de retour conforme a la forme `proc_decl`.

Ce qui se passe a l'execution. `add(1,2)=3`.

Etape 2. Construire une entree programme explicite.

```vit
entry main at core/app {
  return 0
}
```

Pourquoi cette etape est solide. La grammaire impose un point d'entree nomme et une localisation module explicite.

Ce qui se passe a l'execution. Le programme quitte immediatement avec le code `0`.

Etape 3. Construire un `match` avec repli.

```vit
proc unwrap_or_zero(r: int) -> int {
  match r {
    case 0 { give 0 }
    otherwise { give r }
  }
}
```

Pourquoi cette etape est solide. La presence de `otherwise` ferme l'arbre de cas et rend la lecture du flux exhaustive.

Ce qui se passe a l'execution. `unwrap_or_zero(0)=0` et `unwrap_or_zero(7)=7`.

Ce que vous devez maitriser en sortie de chapitre. Les formes syntaxiques critiques sont maitrisees, ce qui reduit les erreurs de structure avant type-check.
