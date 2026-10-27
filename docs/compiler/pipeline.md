# Pipeline de compilation actif

Le bootstrap C fournit encore l'exécutable de stage0 et la frontière avec le
système hôte. Le build standard compile aussi le pipeline natif Vitte depuis
`src/compiler/` et le lie à `vitte`; le compilateur n'est pas encore
auto-hébergé.

## Parcours ordinaire

Pour `compile`, `run`, `check`, `--emit-c` et `--emit-ir`, le CLI transmet
l'entrée au pipeline natif écrit en Vitte. Après le lexer, le parser et
l'analyse sémantique, le pipeline abaisse et valide le HIR puis l'IR. Pour les
constructions couvertes par l'IR, le backend C17 natif émet le programme
depuis l'IR scellé. Les formes, les boucles `for`, les expressions `if` à
valeur et `match` gardent le backend AST pendant que leur lowering IR est
incomplet. Le driver C reçoit le code généré et appelle le compilateur C hôte
pour produire un objet, lier l'exécutable ou lancer le programme.

## Parcours encore sur le bootstrap

- Les commandes d'inspection gardent des étapes C du bootstrap. `check` et
  `--emit-ir` utilisent le pipeline natif, mais `--emit-ir` ne couvre que les
  constructions que le lowering HIR/IR accepte.
- Les diagnostics natifs sont autoritaires pour `compile`, `run`, `check` et
  les sorties IR; le CLI ne relance pas l'analyse bootstrap en cas d'échec.
- Le compileur C et l'éditeur de liens restent des outils hôtes appelés depuis
  le driver C. Le build standard utilise encore stage0 pour traduire les
  modules Vitte en C; aucune reconstruction complète du compilateur par une
  génération issue de ce cycle n'est établie.

La grammaire normative est [`grammar/vitte.ebnf`](../../grammar/vitte.ebnf).
Sa présence ne garantit pas l'acceptation ou la génération de code par chaque
route. Voir la [couverture du parser natif](../bootstrap/frontend_native_port.md)
et le [contrat réduit du seed](selfhost_core_subset.md).

Voir aussi la [carte des sources du compilateur](../MODULE_REFERENCE.md).
