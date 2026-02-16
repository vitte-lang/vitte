# 14. Macros

Ce chapitre poursuit un objectif net: vous donner une maitrise adulte des macros Vitte, c'est-a-dire la capacite de factoriser du code sans introduire une zone grise dans la lecture du programme. Une macro n'est pas un gadget stylistique. C'est un outil de compression syntaxique qui doit rester entierement comprenable par la personne qui relit le code six mois plus tard sous pression de production.

Etape 1. Commencer par une macro volontairement neutre.

```vit
macro nop() {
  let _: int = 0
}
```

Pourquoi cette etape est solide. Cette premiere macro n'a pas d'ambition metier, et c'est exactement ce qui la rend pedagogiquement correcte. Elle fixe la mecanique d'expansion sans bruit annexe. Vous verifiez d'abord la forme, puis seulement ensuite la puissance. En pratique, cette discipline evite le piege classique qui consiste a empiler des macros complexes avant d'avoir stabilise les conventions de lecture de l'equipe.

Ce qui se passe a l'execution. L'appel `nop()` n'ajoute aucune logique fonctionnelle. Le flux observable du programme reste identique, ce qui permet d'isoler le comportement de la macro elle-meme.

Etape 2. Injecter une garde reutilisable dans une procedure sensible.

```vit
macro guard_nonzero(x) {
  if x == 0 { return -1 }
}

proc safe_div(num: int, den: int) -> int {
  guard_nonzero(den)
  give num / den
}
```

Pourquoi cette etape est solide. Ici, la macro devient un vrai levier de conception. Vous retirez une garde critique repetee dans plusieurs procedures, tout en conservant une intention lisible au point d'appel. La valeur de `guard_nonzero` n'est pas de gagner trois lignes; sa valeur est de garantir une politique uniforme sur un risque concret, la division invalide. Lorsque ce motif est centralise, un changement de politique se fait en un seul endroit, et la preuve de coherence de l'ensemble devient immediate.

Ce qui se passe a l'execution. Sur `safe_div(10,2)`, la garde laisse passer et la procedure retourne `5`. Sur `safe_div(10,0)`, la garde coupe le flux avant l'operation arithmetique et retourne `-1`. La frontiere d'erreur est placee exactement la ou le risque apparait.

Etape 3. Encapsuler une normalisation locale sans dissoudre la responsabilite.

```vit
macro clamp01(v) {
  if v < 0 { set v = 0 }
  if v > 1 { set v = 1 }
}

proc normalize01(x: int) -> int {
  let v: int = x
  clamp01(v)
  give v
}
```

Pourquoi cette etape est solide. Cette macro montre un cas legitime de mutation: une variable locale, une portee courte, un contrat clair. Vous ne deleguez pas le raisonnement metier a la macro; vous deleguez seulement un motif de normalisation qui resterait identique partout. La regle d'or est la suivante: une macro est acceptable tant qu'un lecteur peut reconstruire mentalement l'expansion sans effort disproportionne.

Ce qui se passe a l'execution. `normalize01(-3)` retourne `0`, `normalize01(0)` retourne `0`, `normalize01(5)` retourne `1`. La sortie est bornee dans `[0,1]`, et cette garantie reste visible dans la logique source.

Ce que vous devez maitriser en sortie de chapitre. Une macro Vitte utile n'est ni mystique ni decorative. Elle rend une regle recurrente plus stable, plus auditable, et plus facile a faire evoluer. Si elle masque l'intention, supprimez-la. Si elle clarifie un contrat repete, gardez-la et documentez-la comme une piece d'architecture.
