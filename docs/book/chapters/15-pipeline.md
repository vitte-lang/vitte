# 15. Pipeline compilateur

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Lire un programme Vitte comme une suite d'etapes de compilation pour identifier rapidement la nature d'une erreur.

Etape 1. Cas qui traverse parsing, resolution et type-check.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Pourquoi cette etape est solide. La syntaxe est correcte, les symboles sont resolus et les types sont compatibles.

Ce qui se passe a l'execution. Si le binaire est produit, `add(1,2)=3`.

Etape 2. Cas syntaxiquement valide mais type-invalide.

```vit
proc bad(a: int) -> int {
  give a + "x"
}
```

Pourquoi cette etape est solide. Le parsing passe, puis le type-check echoue sur `+` heterogene. Le diagnostic est donc semantique, pas grammatical.

Ce qui se passe a l'execution. Aucune execution. La compilation s'arrete avant emission.

Etape 3. Cas de symbole introuvable.

```vit
proc call_unknown() -> int {
  give missing_fn(1)
}
```

Pourquoi cette etape est solide. L'analyse syntaxique est valide mais la resolution de symboles echoue. L'erreur est localisee au nom non defini.

Ce qui se passe a l'execution. Aucune execution. Le compilateur rejette le module.

Ce que vous devez maitriser en sortie de chapitre. Vous distinguez erreur de syntaxe, erreur de resolution et erreur de type avant d'ouvrir le debugger.
