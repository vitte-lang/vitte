# 18. Tests et validation

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Ecrire des tests Vitte qui verrouillent le contrat nominal, les bornes et la non-regression.

Etape 1. Tester une fonction de saturation.

```vit
proc clamp(x: int, lo: int, hi: int) -> int {
  if x < lo { give lo }
  if x > hi { give hi }
  give x
}
```

Pourquoi cette etape est solide. Trois classes de cas couvrent le contrat: sous borne, interieur, sur borne.

Ce qui se passe a l'execution. `clamp(-1,0,10)=0`, `clamp(5,0,10)=5`, `clamp(99,0,10)=10`.

Etape 2. Verrouiller les frontieres d'un parseur.

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

Pourquoi cette etape est solide. Les tests critiques sont `-1`, `0`, `65535`, `65536`. Ils couvrent toutes les cassures de domaine.

Ce qui se passe a l'execution. `parse_port(0)=0`, `parse_port(65535)=65535`, `parse_port(65536)=-1`.

Etape 3. Capturer une regression sur division.

```vit
proc non_reg_demo(x: int) -> int {
  if x == 0 { give 0 }
  give 10 / x
}
```

Pourquoi cette etape est solide. Le cas `x=0` est un test de securite obligatoire. Il evite le retour d'un bug historique.

Ce qui se passe a l'execution. `non_reg_demo(0)=0`, `non_reg_demo(2)=5`.

Ce que vous devez maitriser en sortie de chapitre. Vos tests couvrent nominal, bornes et historiques de bugs avec des attentes explicites.
