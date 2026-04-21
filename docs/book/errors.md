# Erreurs Vitte

Cette page est la référence stable pour les diagnostics et les corrections courantes.

Carte des couches :
- `parse` : vérifications de syntaxe et de structure.
- `resolve` : vérifications de nom, type, import et déclaration.
- `IR` : vérifications de lowering et de représentation intermédiaire.
- `backend` : vérifications de génération de code et de chaîne d'outils.

Chaque diagnostic suit la même structure :
- Code
- Symptôme
- Cause probable
- Correction
- Exemple
- Voir aussi

<a id="e0001"></a>
## E0001 - identifiant attendu

- Code: `E0001`
- Couche: `parse`
- Symptôme: le parser attendait un nom de variable, de type, de module ou de déclaration.
- Cause probable: une déclaration manque de son identifiant, ou le jeton après un mot-clé n'est pas un nom valide.
- Correction: ajouter un identifiant valide à l'endroit indiqué par le parser.
- Exemple:
```vit
proc main() -> i32 { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0002"></a>
## E0002 - expression attendue

- Code: `E0002`
- Couche: `parse`
- Symptôme: le parser attendait une expression à cet emplacement.
- Cause probable: une valeur, un appel ou une expression de bloc manque après un mot-clé ou un opérateur.
- Correction: fournir une expression comme `1`, `name`, `call()` ou `{ ... }`.
- Exemple:
```vit
let x = 1
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0003"></a>
## E0003 - motif attendu

- Code: `E0003`
- Couche: `parse`
- Symptôme: le parser attendait un motif.
- Cause probable: un construct `when` ou similaire à `match` a reçu un motif incomplet.
- Correction: utiliser un motif comme un identifiant ou un constructeur, par exemple `Some(x)`.
- Exemple:
```vit
when x is Option.Some { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0004"></a>
## E0004 - type attendu

- Code: `E0004`
- Couche: `parse`
- Symptôme: le parser attendait un nom de type.
- Cause probable: une annotation de type est présente, mais le membre de droite manque ou est mal formé.
- Correction: utiliser un type intégré ou un type nommé, par exemple `int` ou `Option[T]`.
- Exemple:
```vit
proc id(x: int) -> int { return x }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0005"></a>
## E0005 - `end` attendu

- Code: `E0005`
- Couche: `parse`
- Symptôme: un bloc a été ouvert mais pas fermé avec `end` ou `.end`.
- Cause probable: un bloc `proc`, `form` ou `pick` manque de son terminateur.
- Correction: ajouter le terminateur manquant pour le construct ouvert.
- Exemple:
```vit
form Point
  field x as int
.end
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0006"></a>
## E0006 - `proc` attendu après l'attribut

- Code: `E0006`
- Couche: `parse`
- Symptôme: un attribut doit être suivi d'une déclaration `proc`.
- Cause probable: l'attribut est accroché au mauvais élément ou se trouve seul.
- Correction: placer l'attribut directement au-dessus d'un `proc`.
- Exemple:
```vit
#[inline]
proc add(a: int, b: int) -> int { return a + b }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0007"></a>
## E0007 - déclaration top-level attendue

- Code: `E0007`
- Couche: `parse`
- Symptôme: le parser attendait une déclaration top-level.
- Cause probable: un élément apparaît à la racine du fichier, mais ce n'est pas l'une des déclarations supportées.
- Correction: limiter le top-level à `space`, `use`, `form`, `pick`, `type`, `const`, `proc` et `entry`.
- Exemple:
```vit
space my/app
proc main() -> int { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0008"></a>
## E0008 - liaison de motif dupliquée

- Code: `E0008`
- Couche: `parse`
- Symptôme: un motif a lié le même nom plus d'une fois.
- Cause probable: le motif réutilise une liaison à deux emplacements.
- Correction: donner un nom distinct à chaque liaison.
- Exemple:
```vit
when Pair(x, x) { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0009"></a>
## E0009 - type inconnu

- Code: `E0009`
- Couche: `resolve`
- Symptôme: un nom de type référencé n'a pas été trouvé.
- Cause probable: le type est mal orthographié ou non importé.
- Correction: vérifier l'orthographe ou importer le type avec `use` ou `pull`.
- Exemple:
```vit
use std/core/option.Option
proc f(x: Option[int]) -> int { return 0 }
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0010"></a>
## E0010 - type de base générique inconnu

- Code: `E0010`
- Couche: `resolve`
- Symptôme: le type de base d'un générique n'a pas été trouvé.
- Cause probable: le type de base générique est mal orthographié ou non importé.
- Correction: vérifier l'orthographe ou importer le type de base avec `use` ou `pull`.
- Exemple:
```vit
use std/core/option.Option
let x: Option[int] = Option.None
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0011"></a>
## E0011 - un type générique requiert au moins un argument

- Code: `E0011`
- Couche: `resolve`
- Symptôme: un type générique a été écrit sans argument de type.
- Cause probable: l'application du type est incomplète.
- Correction: fournir un ou plusieurs arguments de type entre `[ ]`.
- Exemple:
```vit
let x: Option = Option.None
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0012"></a>
## E0012 - type non pris en charge

- Code: `E0012`
- Couche: `resolve`
- Symptôme: cette forme de type n'est pas encore prise en charge.
- Cause probable: la syntaxe de type est assez valide pour être parsée, mais le compilateur n'accepte pas encore cette forme.
- Correction: utiliser une forme de type prise en charge comme les types intégrés, les types nommés, les pointeurs, les slices ou les types de proc.
- Exemple:
```vit
let p: *int = &value
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0013"></a>
## E0013 - identifiant inconnu

- Code: `E0013`
- Couche: `resolve`
- Symptôme: un nom référencé n'a pas été trouvé dans la portée courante.
- Cause probable: le symbole est mal orthographié, hors portée ou non importé.
- Correction: vérifier l'orthographe ou l'importer depuis un module avec `use` ou `pull`.
- Exemple:
```vit
use std/bridge/print.print
proc main() -> int { print("hi"); return 0 }
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0014"></a>
## E0014 - invocation sans callee

- Code: `E0014`
- Couche: `parse`
- Symptôme: une invocation n'a pas de callee.
- Cause probable: l'expression d'appel commence par des arguments au lieu d'un nom de fonction ou de proc.
- Correction: fournir un nom de fonction ou de proc avant les arguments.
- Exemple:
```vit
print("hi")
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0015"></a>
## E0015 - expression non prise en charge dans le HIR

- Code: `E0015`
- Couche: `IR`
- Symptôme: cette expression n'est pas encore prise en charge par le lowering HIR.
- Cause probable: la forme d'expression atteint le lowering, mais le chemin de lowering ne l'implémente pas encore.
- Correction: réécrire l'expression avec des constructeurs pris en charge.
- Exemple:
```vit
let x = value
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0016"></a>
## E0016 - motif non pris en charge dans le HIR

- Code: `E0016`
- Couche: `IR`
- Symptôme: ce motif n'est pas encore pris en charge par le lowering HIR.
- Cause probable: le motif atteint le lowering, mais le chemin de lowering ne l'implémente pas encore.
- Correction: réécrire le motif avec des constructeurs pris en charge.
- Exemple:
```vit
when x is Option.Some { return 0 }
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0017"></a>
## E0017 - instruction non prise en charge dans le HIR

- Code: `E0017`
- Couche: `IR`
- Symptôme: cette instruction n'est pas encore prise en charge par le lowering HIR.
- Cause probable: la forme d'instruction atteint le lowering, mais le chemin de lowering ne l'implémente pas encore.
- Correction: réécrire l'instruction avec des constructeurs pris en charge.
- Exemple:
```vit
return 0
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0018"></a>
## E0018 - un `proc` extern ne peut pas avoir de corps

- Code: `E0018`
- Couche: `resolve`
- Symptôme: une procédure extern ne peut pas définir de corps.
- Cause probable: `#[extern]` et un corps ont été combinés sur le même proc.
- Correction: supprimer le corps ou retirer `#[extern]` si vous voulez l'implémenter ici.
- Exemple:
```vit
#[extern]
proc puts(s: string) -> int
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0019"></a>
## E0019 - un `proc` requiert un corps sauf s'il est marqué `#[extern]`

- Code: `E0019`
- Couche: `resolve`
- Symptôme: une procédure doit avoir un corps sauf si elle est marquée `#[extern]`.
- Cause probable: la déclaration se termine sans corps ou le marqueur extern manque.
- Correction: ajouter un corps avec `{ ... }` ou marquer le proc `#[extern]`.
- Exemple:
```vit
proc add(a: int, b: int) -> int { return a + b }
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0020"></a>
## E0020 - un alias de type requiert un type cible

- Code: `E0020`
- Couche: `resolve`
- Symptôme: un alias de type doit préciser un type cible.
- Cause probable: la définition de l'alias s'arrête après le nom.
- Correction: fournir le membre de droite de l'alias.
- Exemple:
```vit
type Size = int
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0021"></a>
## E0021 - un type générique requiert au moins un argument de type

- Code: `E0021`
- Couche: `resolve`
- Symptôme: un type générique a besoin d'au moins un argument de type.
- Cause probable: l'application générique est vide.
- Correction: fournir des arguments de type entre `[ ]`.
- Exemple:
```vit
let xs: List = List.empty()
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0022"></a>
## E0022 - forme de type HIR inattendue

- Code: `E0022`
- Couche: `IR`
- Symptôme: le compilateur a rencontré une forme de type HIR inattendue.
- Cause probable: le lowering a atteint une forme de type qui n'est pas gérée correctement.
- Correction: essayer un type plus simple et signaler le problème s'il se reproduit.
- Exemple:
```vit
let x: int = 0
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0023"></a>
## E0023 - forme d'expression HIR inattendue

- Code: `E0023`
- Couche: `IR`
- Symptôme: le compilateur a rencontré une forme d'expression HIR inattendue.
- Cause probable: le lowering a atteint une forme d'expression qui n'est pas gérée correctement.
- Correction: essayer une expression plus simple et signaler le problème s'il se reproduit.
- Exemple:
```vit
let x = 1
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0024"></a>
## E0024 - `select` requiert au moins une branche `when`

- Code: `E0024`
- Couche: `resolve`
- Symptôme: une instruction `select` a besoin d'au moins une branche `when`.
- Cause probable: le bloc `select` a été ouvert mais pas peuplé de branches valides.
- Correction: ajouter une clause `when` et éventuellement `otherwise`.
- Exemple:
```vit
select x
  when int(v) { return v }
otherwise { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0025"></a>
## E0025 - une branche `select` doit être une instruction `when`

- Code: `E0025`
- Couche: `resolve`
- Symptôme: chaque branche `select` doit être une instruction `when`.
- Cause probable: une branche utilise une forme d'instruction non autorisée à cet emplacement.
- Correction: remplacer la branche par un motif `when` ou utiliser `otherwise`.
- Exemple:
```vit
select x
  when int(v) { return v }
otherwise { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0026"></a>
## E0026 - forme d'instruction HIR inattendue

- Code: `E0026`
- Couche: `IR`
- Symptôme: le compilateur a rencontré une forme d'instruction HIR inattendue.
- Cause probable: le lowering a rencontré une forme d'instruction qui ne devrait pas atteindre ce chemin.
- Correction: essayer une instruction plus simple et signaler un bug compilateur si cela se reproduit.
- Exemple:
```vit
return 0
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0027"></a>
## E0027 - forme de motif HIR inattendue

- Code: `E0027`
- Couche: `IR`
- Symptôme: le compilateur a rencontré une forme de motif HIR inattendue.
- Cause probable: le lowering a rencontré une forme de motif qui ne devrait pas atteindre ce chemin.
- Correction: essayer un motif plus simple et signaler un bug compilateur si cela se reproduit.
- Exemple:
```vit
when x is Option.None { return 0 }
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0028"></a>
## E0028 - forme de déclaration HIR inattendue

- Code: `E0028`
- Couche: `IR`
- Symptôme: le compilateur a rencontré une forme de déclaration HIR inattendue.
- Cause probable: le lowering a rencontré une forme de déclaration qui ne devrait pas atteindre ce chemin.
- Correction: essayer une déclaration plus simple et signaler un bug compilateur si cela se reproduit.
- Exemple:
```vit
proc main() -> int { return 0 }
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`
