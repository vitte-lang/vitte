# 20. Reproductibilite

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Garantir qu'un programme Vitte produit le meme resultat a entree identique, sur chaque execution.

Etape 1. Fonction pure de base.

```vit
proc stable(x: int) -> int {
  give x * 2 + 1
}
```

Pourquoi cette etape est solide. Aucun etat externe ni source non deterministe. Le resultat est une consequence directe de `x`.

Ce qui se passe a l'execution. `stable(10)=21` sur toute machine compatible.

Etape 2. Variabilite injectee explicitement.

```vit
proc with_seed(seed: int) -> int {
  give seed
}
```

Pourquoi cette etape est solide. Le parametre remplace toute dependance implicite au temps ou a l'environnement.

Ce qui se passe a l'execution. `with_seed(12345)=12345` de maniere stable.

Etape 3. Sequence deterministic de checksum.

```vit
proc repro_checksum(seed: int, n: int) -> int {
  let i: int = 0
  let acc: int = seed
  loop {
    if i >= n { break }
    set acc = (acc * 31 + i) % 104729
    set i = i + 1
  }
  give acc
}
```

Pourquoi cette etape est solide. La recurrence est entierement definie par les entrees et une borne de boucle explicite.

Ce qui se passe a l'execution. `(5,4)` evolue `155`, `4806`, `44114`, `57592` puis retourne `57592`.

Ce que vous devez maitriser en sortie de chapitre. Les entrees sont explicites, la boucle est bornee et aucune source d'aleatoire cachee n'apparait.
