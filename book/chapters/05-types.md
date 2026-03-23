# 5. Types et valeurs

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/04-syntaxe.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/04-syntaxe.md`, `book/chapters/06-procedures.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Types et valeurs**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Types et valeurs**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Types et valeurs**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Types et valeurs** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Types et valeurs**.

## Exercice court

Prenez un exemple du chapitre sur **Types et valeurs**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Types et valeurs**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 5.0 Types primitifs (table canonique)

Règle explicite de ce livre:
- `int` est le type entier canonique pour les exemples pédagogiques.
- quand la largeur binaire fait partie du contrat, utiliser `i32/i64/i128/u32/u64/u128`.

| Type | Taille | Signé / non signé | Cas d’usage principal | Exemple |
| --- | --- | --- | --- | --- |
| `bool` | 1 bit logique (représentation backend dépendante) | non signé | conditions, garde, test | `let ok: bool = true` |
| `string` | variable | n/a | texte et messages | `let msg: string = "ok"` |
| `int` | dépend de l’implémentation cible | signé | calcul général en chapitre débutant | `let n: int = 42` |
| `i32` | 32 bits | signé | protocole binaire, API bornée 32 bits | `let code: i32 = 200` |
| `i64` | 64 bits | signé | horodatage, compteurs longs | `let ts: i64 = 1700000000` |
| `i128` | 128 bits | signé | identifiants/calculettes très grands | `let big: i128 = 123456789` |
| `u32` | 32 bits | non signé | tailles, masques, registres 32 bits | `let mask: u32 = 255` |
| `u64` | 64 bits | non signé | offsets/fichiers/protocoles 64 bits | `let size: u64 = 4096` |
| `u128` | 128 bits | non signé | IDs larges, hash partiels | `let id: u128 = 1` |

## 5.0.1 Quand éviter `int`

Éviter `int` quand la taille doit rester stable entre machines:
- API binaire et liaison native.
- protocole réseau et format de fichier.
- sérialisation persistée.
- registres matériels et code kernel/bare-metal.

Dans ces cas, imposer une largeur explicite (`i32/i64/i128/u32/u64/u128`) et conserver cette largeur de bout en bout.

Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `book/checklist-editoriale.md`. Complément: `book/erreurs-classiques.md`.
## 5.1 Décrire une donnée avec un type structurel

Un type structurel regroupe plusieurs champs nommés qui forment une seule entité logique. Il ne s'agit plus de "deux entiers quelconques", mais d'un point du plan avec un sens métier explicite.

```vit
form Point {
  x: int
  y: int
}
proc manhattan(p: Point) -> int {
  give p.x + p.y
}
```

Lecture ligne par ligne (débutant):
1. `form Point {` -> Comportement: cette ligne ouvre la structure `Point` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. -> Preuve: plusieurs fonctions peuvent manipuler `Point` sans redéfinir ses champs.
2. `x: int` -> Comportement: cette ligne déclare le champ `x` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
3. `y: int` -> Comportement: cette ligne déclare le champ `y` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
4. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc manhattan(p: Point) -> int {` -> Comportement: le contrat est défini pour `manhattan`: entrées `p: Point` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `manhattan` retourne toujours une valeur compatible avec `int`.
6. `give p.x + p.y` -> Comportement: la branche renvoie immédiatement `p.x + p.y` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `p.x + p.y`.
7. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `p.x + p.y`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

`Point` exprime une idée claire: une valeur avec deux coordonnées `x` et `y`. La procédure `manhattan` n'accepte donc pas n'importe quelles données; elle exige un `Point`. Cette contrainte est précieuse, car elle empêche les appels ambigus et rend l'intention évidente dans la signature.

À l'exécution, le comportement reste simple:
- avec `Point(3,4)`, la fonction renvoie `7`.
- avec `Point(10,-2)`, elle renvoie `8`.

Mais la vraie valeur de l'exemple n'est pas le calcul en lui-même. Elle est dans la lisibilité du contrat: en voyant `manhattan(p: Point)`, le lecteur comprend immédiatement la nature de la donnée attendue.

Réflexe à garder: quand une donnée a un sens métier, donnez-lui un type métier. Vous évitez ainsi les échanges implicites de "deux entiers qui se ressemblent" mais ne veulent pas dire la même chose.

Erreurs fréquentes à éviter:
- manipuler des tuples ou des entiers séparés au lieu d'un type métier explicite.
- donner des noms de champs trop vagues, qui masquent l'intention.
- multiplier les conversions inutiles autour d'un type déjà bien défini.

## 5.2 Représenter des alternatives avec un type somme

Certaines valeurs n'ont pas une seule forme possible. Un résultat peut réussir, ou échouer. Un parseur peut produire une valeur, ou une erreur. Le type somme sert précisément à encoder ces alternatives sans ambiguïté.

```vit
pick Result {
  case Ok(value: int)
  case Err(code: int)
}
```

Lecture ligne par ligne (débutant):
1. `pick Result {` -> Comportement: cette ligne ouvre le type fermé `Result` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. -> Preuve: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Ok(value: int)` -> Comportement: ce cas décrit `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Ok(value: int)`, ce bloc devient le chemin actif.
3. `case Err(code: int)` -> Comportement: ce cas décrit `Err(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Err(code: int)`, ce bloc devient le chemin actif.
4. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(value: int)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ici, `Result` décrit un univers fermé d'états valides: soit `Ok` avec une valeur entière, soit `Err` avec un code d'erreur. Ce point est essentiel. Le programme n'a plus à deviner l'état réel d'une donnée à partir de conventions implicites; l'état est nommé et porté par le type.

À l'exécution, toute valeur de type `Result` est nécessairement l'une des deux variantes. Vous ne pouvez pas "inventer" un troisième état non prévu sans modifier le type lui-même. Cette fermeture réduit les états impossibles et simplifie la maintenance: lorsqu'un comportement change, vous savez exactement quelles formes de données sont concernées.

En pratique, ce modèle améliore aussi les messages d'erreur et les tests:
- les tests ciblent des variantes explicites.
- les erreurs métiers restent nommées.
- les cas non gérés deviennent visibles plus tôt.

Erreurs fréquentes à éviter:
- utiliser un entier "sentinelle" (par exemple `-1`) à la place d'une variante d'erreur.
- mélanger dans un même champ des données de nature différente.
- introduire des cas implicites que le type ne documente pas.

## 5.3 Traiter une variante avec `match`

Une fois le type somme défini, la lecture correcte consiste à traiter explicitement chaque variante. Le `match` aligne le contrôle de flux sur la structure du type.

```vit
proc unwrap_or_zero(r: Result) -> int {
  match r {
    case Ok(v) { give v }
    case Err(_) { give 0 }
    otherwise { give 0 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `proc unwrap_or_zero(r: Result) -> int {` -> Comportement: le contrat est posé pour `unwrap_or_zero`: entrées `r: Result` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `unwrap_or_zero` retourne toujours une valeur compatible avec `int`.
2. `match r {` -> Comportement: cette ligne démarre un dispatch déterministe sur `r`: une seule branche sera choisie selon la forme de la valeur analysée. -> Preuve: pour la même valeur de `r`, la même branche sera toujours exécutée.
3. `case Ok(v) { give v }` -> Comportement: ce cas décrit `Ok(v)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Ok(v)`, ce bloc devient le chemin actif.
4. `case Err(_) { give 0 }` -> Comportement: ce cas décrit `Err(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. -> Preuve: si la valeur analysée correspond à `Err(_)`, ce bloc devient le chemin actif.
5. `otherwise { give 0 }` -> Comportement: cette ligne définit un chemin de secours explicite. -> Preuve: si aucun `case` ne correspond, `give 0` est exécuté pour garantir une sortie stable.
6. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(v)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

La fonction `unwrap_or_zero` exprime une règle concrète: si le résultat est `Ok`, on récupère sa valeur; s'il s'agit d'une erreur, on renvoie `0`. Le flux suit exactement les variantes de `Result`, ce qui rend le comportement immédiat à vérifier.

Lecture pas à pas:
- si `r = Ok(42)`, la première branche correspond, retour `42`.
- si `r = Err(500)`, la deuxième branche correspond, retour `0`.
- la branche `otherwise` sert ici de filet de sécurité.

Le code ne "devine" rien: il inspecte la forme de la valeur, puis applique la règle associée. C'est cette correspondance entre type et branches qui rend le système robuste.

Erreurs fréquentes à éviter:
- traiter seulement une variante et oublier les autres.
- écrire des branches qui contredisent le sens métier des cas.
- ajouter un `otherwise` pour masquer un oubli de conception plutôt que corriger le modèle.

## À retenir

Le typage devient réellement utile lorsqu'il porte le métier, pas seulement la forme technique des données. Un type structurel clarifie ce qu'est une entité. Un type somme clarifie les états possibles de cette entité. Un `match` bien écrit clarifie comment on réagit à chacun de ces états. Avec ces trois outils, votre code gagne en lisibilité, en sécurité et en stabilité: il devient plus difficile d'exprimer une erreur, et plus facile d'expliquer une intention.

Critère pratique de qualité pour ce chapitre:
- vous pouvez dire ce que représente chaque type en une phrase claire.
- vous savez lister toutes les variantes possibles d'un résultat.
- vous pouvez relire un `match` et vérifier qu'aucun cas important n'est oublié.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/case.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.
- `book/keywords/if.md`.
- `book/keywords/int.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement concret (sans répétition)

### 1. Snippet de référence

```vit
form Point {
  x: int
  y: int
}
proc manhattan(p: Point) -> int {
  give p.x + p.y
}
```

### 2. Lecture du code ligne par ligne

1. `form Point {` -> participe au flux principal du traitement.
2. `x: int` -> participe au flux principal du traitement.
3. `y: int` -> participe au flux principal du traitement.
4. `}` -> participe au flux principal du traitement.
5. `proc manhattan(p: Point) -> int {` -> déclare un contrat clair entre entrées et sortie.
6. `give p.x + p.y` -> rend la sortie observable sans ambiguïté.
7. `}` -> participe au flux principal du traitement.

### 3. Exécution réelle (entrée -> traitement -> sortie)

1. Entrée: préciser les valeurs acceptées et refusées.
2. Traitement: suivre le chemin nominal, puis la première garde.
3. Sortie: vérifier la valeur retournée ou l'erreur attendue.

### 4. Cas limite et erreur volontaire

- Cas limite: forcer la garde et confirmer la sortie de secours.
- Cas erreur: injecter un type inattendu et lire le diagnostic exact.
- Correction: modifier une seule ligne, recompiler, valider.

### 5. Refactor concret à faible risque

Méthode: garder la signature, simplifier une branche, et prouver que le comportement reste identique avec un test nominal + un test limite.

### 6. Série de scénarios représentatifs

Cas 1: pour **types et valeurs**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 2: pour **types et valeurs**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 3: pour **types et valeurs**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 4: pour **types et valeurs**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 5: pour **types et valeurs**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 6: pour **types et valeurs**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 7: pour **types et valeurs**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 8: pour **types et valeurs**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 9: pour **types et valeurs**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 10: pour **types et valeurs**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 11: pour **types et valeurs**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 12: pour **types et valeurs**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 13: pour **types et valeurs**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 14: pour **types et valeurs**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 15: pour **types et valeurs**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 16: pour **types et valeurs**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 17: pour **types et valeurs**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 18: pour **types et valeurs**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.

### 7. Checklist finale de compréhension

1. Le contrat d'entrée est explicite.
2. Le cas nominal est testable sans ambiguïté.
3. Le cas limite est traité explicitement.
4. Le diagnostic d'erreur est actionnable.
5. Le corrigé suit une modification locale et vérifiable.

<!-- AUTO_EXPANSION_V1 END -->

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **types et valeurs**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
form Point {
  x: int
  y: int
}
proc manhattan(p: Point) -> int {
  give p.x + p.y
}
```

Lecture ligne par ligne:
1. `form Point {` -> participe au déroulé du traitement.
2. `x: int` -> participe au déroulé du traitement.
3. `y: int` -> participe au déroulé du traitement.
4. `}` -> participe au déroulé du traitement.
5. `proc manhattan(p: Point) -> int {` -> pose un contrat clair de fonction.
6. `give p.x + p.y` -> renvoie la sortie vérifiable.
7. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
3. Vérifier la nouvelle sortie sur une entrée limite.

### Exemple C: bug reproductible puis correction locale

Procédure:
1. Introduire une incompatibilité de type sur un appel.
2. Compiler et lire le premier diagnostic.
3. Corriger une seule ligne (pas de refactor global).
4. Recompiler et vérifier le retour nominal.

### Résultat attendu

- Le lecteur comprend ce que fait le code sans abstraction inutile.
- Chaque exemple est relié à une action concrète.
- La correction est reproductible et testable.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->
