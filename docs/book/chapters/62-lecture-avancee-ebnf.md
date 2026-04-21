# 62. Lecture avancee de l'EBNF du langage

Niveau: Avancé

Prérequis: `docs/book/chapters/27-grammaire.md`, `docs/book/grammar/precedence.md`.
Voir aussi: à définir.

## Problème Concret

Situation réelle: pour Lecture avancee de l'EBNF du langage, la question n'est pas 'quella règle écrire' mais 'quel chemin le code prend vraiment'. Cette lecture par exécution évite les interprétations vagues.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: vous retrouvez le même pipeline pour observer ce qui change réellement quand on modifie une branche.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Objectif

Lire rapidement une regle EBNF et deduire contraintes de parse et implications AST.

## Methode

1. Partir des non-terminaux racine (`program`, `toplevel`, `stmt`, `expr`).
2. Identifier options et repetitions.
3. Repeter avec exemples valid/invalid.
4. Relier regle -> diagnostic utilisateur.

## Checklist

1. Ambiguites potentielles explicitees.
2. Priorites operatoires verifiees.
3. Exemples minimaux associes a chaque regle critique.

## Exemples progressifs (N1 -> N3)

### N1 (base): lire une règle simple

Snippet EBNF:

```vit
let_stmt ::= "let" WS1 ident WS? "=" WS? expr ;
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): alternatives

Snippet EBNF:

```vit
stmt ::= let_stmt | if_stmt | return_stmt ;
```

Commandes:

```bash
make grammar-test
```

### N3 (avancé): priorité opératoire

Snippet EBNF:

```vit
add_expr ::= mul_expr { ("+"|"-") mul_expr } ;
mul_expr ::= unary_expr { ("*"|"/") unary_expr } ;
```

Commandes:

```bash
make grammar-gate
```

### Anti-exemple

```text
Interpréter une règle sans test valid/invalid associé.
```

## Validation rapide

1. Relier la règle à un cas valid.
2. Relier la règle à un cas invalid.
3. Vérifier le diagnostic attendu.

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas d'erreur.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte la règle, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **lecture avancee de l'ebnf du langage**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
let_stmt ::= "let" WS1 ident WS? "=" WS? expr ;
```

Lecture ligne par ligne:
1. `let_stmt ::= "let" WS1 ident WS? "=" WS? expr ;` -> participe au déroulé du traitement.

### Exemple B: variante cas d'erreur (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant un test explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter un test avant cette ligne.
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


## Exemple Étendu


```vit
// Scenario lecture avancee ebnf: execution complete et verifiable
space demo/lecture-avancee-ebnf

form SourceUnit { bytes: int lines: int tokens_hint: int }
pick ParseState { case Parsed(nodes: int) case Failed(code: int) }

// Scan: transforme l'entrée brute en signal exploitable
proc scan(u: SourceUnit) -> int {

  if u.bytes <= 0 { give 0 }

  if u.lines <= 0 { give 0 }

  give (u.tokens_hint + u.lines)
}

// Parse: construit un état syntaxique stable
proc parse(token_count: int) -> ParseState {

  if token_count == 0 { give ParseState.Failed(101) }

  if token_count < 4 { give ParseState.Failed(102) }

  give ParseState.Parsed(token_count)
}

proc validate_structure(nodes: int) -> int {

  if nodes <= 0 { give 201 }

  if nodes > 200000 { give 202 }

  give 0
}

// Conversion finale vers un code de sortie
proc to_exit(p: ParseState) -> int {

  match p {
    case Parsed(n) {
      let v: int = validate_structure(n)

      if v != 0 { give v }

      give 0
    }
    case Failed(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let u: SourceUnit = SourceUnit(120, 12, 18)
  let t: int = scan(u)
  let p: ParseState = parse(t)

  return to_exit(p)
}
```

## Explication détaillée du gros bloc

Ici, l'objectif est de comprendre le chemin réel du programme, ligne par ligne, jusqu'au code de sortie.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `scan`: lit `u: SourceUnit` et renvoie `int`.
- `parse`: lit `token_count: int` et renvoie `ParseState`.
- `validate_structure`: lit `nodes: int` et renvoie `int`.
- `to_exit`: lit `p: ParseState` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `scan` est appelé pour traiter l'étape suivante.
3. `parse` est appelé pour traiter l'étape suivante.
4. `to_exit` est appelé pour traiter l'étape suivante.
5. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `u.bytes <= 0`.
- Test évalué: `u.lines <= 0`.
- Test évalué: `token_count == 0`.
- Test évalué: `token_count < 4`.
- Test évalué: `nodes <= 0`.
- Test évalué: `nodes > 200000`.
- Sélection par `match p`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> scan -> parse -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> scan renvoie un code d'erreur -> sortie non nulle`.

### 5. Pourquoi ce découpage est utile
- Vous testez chaque fonction seule, puis le flux complet.
- Vous savez où modifier une règle sans casser tout le programme.
- Vous pouvez expliquer la sortie en suivant simplement les appels.

### 6. Vérification rapide
1. Relancer avec une entrée normale et noter la sortie.
2. Relancer avec une entrée invalide et vérifier le code d'erreur.
3. Confirmer que la même entrée donne toujours la même sortie.


## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les tests sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas d'erreur réel:
- Entree degradee ou incomplete: le test doit couper le flux tot avec une sortie explicite.

A tester:
- Unité valide -> sortie 0.
- Entrée vide (bytes=0) -> sortie 101.
- Structure surdimensionnée -> sortie 202.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `scan` lit `u: SourceUnit` puis renvoie `int`.
- `parse` lit `token_count: int` puis renvoie `ParseState`.
- `validate_structure` lit `nodes: int` puis renvoie `int`.
- `to_exit` lit `p: ParseState` puis renvoie `int`.

- Variables créées (valeur initiale):
- `v: int` démarre avec `validate_structure(n)`.
- `u: SourceUnit` démarre avec `SourceUnit(120, 12, 18)`.
- `t: int` démarre avec `scan(u)`.
- `p: ParseState` démarre avec `parse(t)`.

- Conditions qui changent le chemin:
- si `u.bytes <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `u.lines <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `token_count == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `token_count < 4` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `nodes <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `nodes > 200000` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `v != 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: v=validate_structure(n) -> u=SourceUnit(120, 12, 18) -> t=scan(u) -> p=parse(t)
- enchaînement: scan -> parse -> to_exit
- sortie finale sur ce chemin: `to_exit(p)`.

- Trace d'erreur (valeurs exemple):
- si `u.bytes <= 0` devient vrai, la fonction renvoie immédiatement `0`.

- Vérification rapide:
- relancer avec une entrée normale et noter la sortie,
- relancer avec une entrée invalide et noter le code d'erreur,
- confirmer qu'une même entrée produit toujours la même sortie.

## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Test absente ou mal ordonnée | Rejouer avec cas d'erreur | Remonter le test avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Règle implicite | Comparer nominal vs limite | Formaliser la règle dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas d'erreur réel et prévoir sa sortie,
- identifier où ajouter un test sans casser le nominal.


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas d'erreur, verrouiller le test au bon niveau.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
