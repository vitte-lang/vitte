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

## Approfondissement guidé

### 1. Ce qu'il faut vraiment retenir

Le coeur de **types et valeurs** est de prendre des décisions lisibles et vérifiables.
Dans un projet réel, la compréhension rapide prime sur la complexité apparente.
L'objectif de cette section est de transformer le chapitre en guide opérationnel,
pas en résumé théorique.

Trois idées pratiques gouvernent ce sujet:
1. faire un changement à la fois;
2. garder des invariants explicites;
3. valider le résultat avec une preuve simple (test, sortie, diagnostic).

### 2. Carte mentale utilisable en équipe

Quand vous travaillez sur **types et valeurs**, posez systématiquement ces questions:
- quel est le contrat d'entrée;
- quel est le résultat attendu;
- quels sont les cas limites visibles;
- quelle erreur doit être compréhensible en moins de 30 secondes.

Cette carte mentale évite les refactors fragiles.
Elle permet aussi d'aligner débutants et profils avancés sur le même langage de travail.

### Étude de cas pratique

Cas: un module lié à **types et valeurs** (05-types.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **types et valeurs** (05-types.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **types et valeurs** (05-types.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

<!-- AUTO_EXPANSION_V1 END -->

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs (par cas d'usage)

Cette section donne des exemples variés et réalistes pour **types et valeurs**.
Objectif: multiplier les angles de lecture sans alourdir le noyau du chapitre.

### Exemple 1: cas nominal minimal

```vit
entry main at app/demo {
  return 0
}
```

Quand l'utiliser: valider la base exécutable avant tout ajout de complexité.

### Exemple 2: garde explicite (cas limite)

```vit
proc clamp_non_negative(x: int) -> int {
  if x < 0 {
    give 0
  }
  give x
}
```

Quand l'utiliser: éviter les comportements implicites sur entrées hors contrat.

### Exemple 3: erreur de type volontaire (diagnostic)

```vit
proc needs_int(x: int) -> int {
  give x
}
entry main at app/demo {
  let s: string = "42"
  return needs_int(s)
}
```

Quand l'utiliser: entraîner la lecture des diagnostics compilateur.

### Exemple 4: séparation module / API

```vit
space app/math
proc add(a: int, b: int) -> int {
  give a + b
}
share add
```

Quand l'utiliser: clarifier ce qui est public vs interne dans l'architecture.

### Exemple 5: flux de contrôle lisible

```vit
entry main at app/demo {
  let n: int = 3
  if n > 0 {
    return 1
  }
  return 0
}
```

Quand l'utiliser: expliciter une décision métier avec un chemin nominal et un fallback.

### Exemple 6: version testable d'une procédure

```vit
proc is_even(x: int) -> bool {
  give x % 2 == 0
}
```

Cas de test conseillés:
1. `is_even(2)` -> `true`.
2. `is_even(3)` -> `false`.
3. `is_even(0)` -> `true`.

Quand l'utiliser: convertir rapidement une règle en contrat vérifiable.

### Exemple 7: refactor sûr (avant/après)

Avant:
```vit
proc parse_port(s: string) -> int {
  give 0
}
```

Après:
```vit
proc parse_port(s: string) -> int {
  if s == "" {
    give 0
  }
  give 8080
}
```

Quand l'utiliser: faire évoluer le comportement sans casser la signature publique.

### Exemple 8: checklist de lecture rapide

1. Où est le contrat d'entrée?
2. Quel est le chemin nominal?
3. Quel est le cas limite traité?
4. Quelle erreur reste explicite?
5. Quel test prouve le comportement?

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->
