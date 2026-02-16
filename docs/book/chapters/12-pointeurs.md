# 12. Pointeurs, references et memoire

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Encadrer le travail memoire en Vitte pour que chaque acces sensible soit protege par des preconditions visibles.

Etape 1. Confiner une instruction machine dans une frontiere courte.

```vit
proc cpu_pause() {
  unsafe {
    asm("pause")
  }
}
```

Pourquoi cette etape est solide. La zone `unsafe` est volontairement minimale. Le reste du code ne propage pas la complexite machine.

Ce qui se passe a l'execution. L'appel execute une pause CPU sans effet de domaine supplementaire.

Etape 2. Lecture bornee dans un buffer.

```vit
form Buffer {
  data: int[]
}

proc read_at(b: Buffer, i: int) -> int {
  if i < 0 { give 0 }
  if i >= b.data.len() { give 0 }
  give b.data[i]
}
```

Pourquoi cette etape est solide. Les bornes sont verifiees avant le dereferencement. Le retour sentinelle `0` est un contrat explicite de cette API.

Ce qui se passe a l'execution. Avec `data=[10,20,30]`, `read_at(...,1)=20` et `read_at(...,9)=0`.

Etape 3. Ecriture bornee avec contrat symetrique.

```vit
proc write_at(b: Buffer, i: int, v: int) -> int {
  if i < 0 { give 0 }
  if i >= b.data.len() { give 0 }
  b.data[i] = v
  give 1
}
```

Pourquoi cette etape est solide. Lire et ecrire partagent la meme garde de bornes. Cette symetrie evite les regressions de coherence memoire.

Ce qui se passe a l'execution. `write_at([10,20,30],1,99)` retourne `1` et produit `[10,99,30]`. `write_at(...,7,99)` retourne `0` sans mutation.

Ce que vous devez maitriser en sortie de chapitre. Toute operation memoire est bornee, toute zone `unsafe` est courte et chaque contrat de retour est stable.
