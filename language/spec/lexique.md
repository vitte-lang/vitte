# Lexique Vitte

## Identifiants
* Autorisés : lettres ASCII (A-Z, a-z) suivies de lettres, chiffres ou `_`.
* Interdiction : commencer par un mot-clé réservé (voir ci-dessous).

## Littéraux
* Nombres : entiers (`42`) ou flottants (`3.1415`).
* Booléens : `true`, `false`.
* Nullité : `null`.
* Chaînes : `"..."` avec échappements habituels.
* Collections : listes `[a, b, c]` et dictionnaires `{cle: valeur}`.

## Indentation
Le lexer produit explicitement `NEWLINE`, `INDENT` et `DEDENT`, permettant de
détecter les blocs significatifs comme en Python. Les blocs peuvent aussi être
encadrés de `{ ... }` pour des générateurs ou du code auto-formaté.

## Mots-clés réservés

```
module, import, export, muffin,
program, software, kernel, service, driver, utility,
scenario, capsule, bridge, device, blueprint, realm, tool, command, route, pipeline,
interrupt, boot, map, port, reg, layout, unsafe, intrinsic, extern, asm,
task, spawn, await, parallel, race, channel,
probe, trace, watch, hook, meta, pragma, emit, explain,
table, plot, ui,
true, false, null, let, const, if, elif, else, while, for, repeat, match, and, or, not
```

Toute tentative d’utiliser l’un de ces mots-clés comme identifiant doit être
signalée par le compilateur dès l’analyse lexicale.
