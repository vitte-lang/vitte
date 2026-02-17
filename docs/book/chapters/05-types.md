# 5. Types et valeurs

Niveau: Débutant

Prérequis: chapitre précédent `docs/book/chapters/04-syntaxe.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/04-syntaxe.md`, `docs/book/chapters/06-procedures.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Dans un code fragile, les règles métier existent souvent "dans la tête" de l'équipe: on sait vaguement ce qu'une valeur représente, on espère qu'elle sera utilisée correctement, et l'on corrige au fil des incidents. Un langage typé permet au contraire d'écrire ces règles dans le programme lui-même. Ce chapitre montre comment passer d'une convention implicite à un contrat explicite, vérifiable dès la lecture et solide à l'exécution.

Nous allons avancer en trois temps. D'abord, décrire une donnée composée avec un type structurel. Ensuite, représenter des états alternatifs avec un type somme. Enfin, consommer ce type somme avec un `match` qui suit sa structure. L'enjeu est simple: faire en sorte que le code dise clairement ce qui est autorisé, et ce qui ne l'est pas.

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


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
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
1. `form Point {` cette ligne ouvre la structure `Point` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `Point` sans redéfinir ses champs.
2. `x: int` cette ligne déclare le champ `x` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
3. `y: int` cette ligne déclare le champ `y` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc manhattan(p: Point) -> int {` ici, le contrat complet est défini pour `manhattan`: entrées `p: Point` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `manhattan` retourne toujours une valeur compatible avec `int`.
6. `give p.x + p.y` ici, la branche renvoie immédiatement `p.x + p.y` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `p.x + p.y`.
7. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


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
1. `pick Result {` cette ligne ouvre le type fermé `Result` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Ok(value: int)` cette ligne décrit le cas `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok(value: int)`, ce bloc devient le chemin actif.
3. `case Err(code: int)` cette ligne décrit le cas `Err(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(code: int)`, ce bloc devient le chemin actif.
4. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


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
1. `proc unwrap_or_zero(r: Result) -> int {` sur cette ligne, le contrat complet est posé pour `unwrap_or_zero`: entrées `r: Result` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `unwrap_or_zero` retourne toujours une valeur compatible avec `int`.
2. `match r {` cette ligne démarre un dispatch déterministe sur `r`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `r`, la même branche sera toujours exécutée.
3. `case Ok(v) { give v }` cette ligne décrit le cas `Ok(v)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok(v)`, ce bloc devient le chemin actif.
4. `case Err(_) { give 0 }` cette ligne décrit le cas `Err(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Err(_)`, ce bloc devient le chemin actif.
5. `otherwise { give 0 }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give 0` est exécuté pour garantir une sortie stable.
6. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


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

- `docs/book/keywords/case.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/give.md`.
- `docs/book/keywords/if.md`.
- `docs/book/keywords/int.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
