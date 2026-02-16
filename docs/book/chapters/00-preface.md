# 0. Preface

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Installer un cadre d'apprentissage progressif pour maitriser Vitte en profondeur, du programme minimal jusqu'aux projets complets.

Etape 1. Comprendre la promesse du langage.

```vit
proc promise(a: int, b: int) -> int {
  give a + b
}
```

Pourquoi cette etape est solide. Vitte privilegie la clarte du contrat sur la magie syntaxique. Le lecteur doit toujours voir ce qui entre, ce qui sort et pourquoi.

Ce qui se passe a l'execution. `promise(20,22)=42`.

Etape 2. Accepter la discipline des invariants.

```vit
proc bounded(x: int) -> int {
  if x < 0 { give 0 }
  if x > 100 { give 100 }
  give x
}
```

Pourquoi cette etape est solide. Un invariant est une regle locale qui evite des classes entieres de bugs. Le livre insiste sur cette pratique dans tous les chapitres.

Ce qui se passe a l'execution. `bounded(-1)=0`, `bounded(70)=70`, `bounded(140)=100`.

Etape 3. Construire une progression orientee projet.

```vit
entry main at core/app {
  let v: int = bounded(140)
  return v
}
```

Pourquoi cette etape est solide. La theorie est toujours raccordee a un flux d'execution concret. Chaque notion abstraite est ramenee a un comportement observable.

Ce qui se passe a l'execution. `main` calcule `bounded(140)` puis termine avec `100`.

Ce que vous devez maitriser en sortie de chapitre. Le ton du livre est technique, le cheminement est progressif et chaque chapitre sert la construction d'un vrai code Vitte.
