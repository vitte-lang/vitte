# 1. Demarrer avec Vitte

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Installer une boucle de travail reproductible: ecrire, verifier, corriger, valider.

Etape 1. Programme minimal executable.

```vit
entry main at core/app {
  return 0
}
```

Pourquoi cette etape est solide. `entry` fixe le point de depart. `return` fixe le code de sortie sans ambiguite.

Ce qui se passe a l'execution. Le programme entre dans `main` puis sort avec `0`.

Etape 2. Introduire une procedure.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}

entry main at core/app {
  let r: int = add(20, 22)
  return r
}
```

Pourquoi cette etape est solide. Signature compile-time stable et appel verifie. Pas d'effet secondaire cache.

Ce qui se passe a l'execution. `add(20,22)=42`, sortie finale `42`.

Etape 3. Ajouter une boucle bornEe.

```vit
proc sum_to(n: int) -> int {
  let i: int = 0
  let s: int = 0
  loop {
    if i > n { break }
    set s = s + i
    set i = i + 1
  }
  give s
}
```

Pourquoi cette etape est solide. Borne de sortie explicite, mutation d'etat localisee.

Ce qui se passe a l'execution. `sum_to(3)` retourne `6`.

Ce que vous devez maitriser en sortie de chapitre. Entree claire, signatures explicites, borne de boucle visible.
