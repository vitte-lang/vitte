# 30. FAQ

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Repondre aux questions frequentes avec des demonstrations Vitte courtes mais rigoureuses.

Etape 1. Pourquoi typer explicitement les signatures.

```vit
proc q_types(x: int) -> int {
  give x
}
```

Pourquoi cette etape est solide. Le typage explicite verrouille contrat d'appel et de retour, ce qui raccourcit la chaine de diagnostic.

Ce qui se passe a l'execution. `q_types(5)=5`.

Etape 2. Quand utiliser `unsafe`.

```vit
proc q_unsafe() -> int {
  unsafe { asm("nop") }
  give 0
}
```

Pourquoi cette etape est solide. `unsafe` est reserve aux operations non prouvables par le compilateur et doit rester confine.

Ce qui se passe a l'execution. Instruction machine `nop`, puis retour `0`.

Etape 3. Comment choisir le minimum de tests utiles.

```vit
proc q_tests(x: int) -> int {
  if x < 0 { give -1 }
  give x
}
```

Pourquoi cette etape est solide. Le contrat impose au moins un cas borne (`x<0`) et un cas nominal (`x>=0`).

Ce qui se passe a l'execution. `q_tests(-1)=-1` et `q_tests(9)=9`.

Ce que vous devez maitriser en sortie de chapitre. Les signatures sont claires, `unsafe` est borne et la strategie de test couvre nominal et frontieres.
