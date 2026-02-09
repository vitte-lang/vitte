# 2. Philosophie et design

Un langage bas niveau doit faire deux choses : ne pas vous mentir et ne pas vous surprendre. Vitte a été pensé autour de cette idée. C’est un langage qui préfère la clarté à la magie, et la stabilité à la nouveauté permanente.

## Principes

Clarté par défaut : le code doit se lire comme un contrat. Diagnostics précis : les messages d’erreur doivent aider, pas juger. Pipeline transparent : comprendre la compilation doit être possible sans lire 100 000 lignes de code. Reproductibilité : un build doit être un résultat, pas une loterie.

Ces principes ne sont pas décoratifs. Ils influencent la syntaxe, les messages d’erreur, et même la manière dont on organise les modules.

## Le coût des abstractions

Les abstractions sont utiles, mais elles ont un coût. Vitte vous demande d’être explicite sur ce coût. Cela rend le code plus verbeux au début, mais il devient plus robuste quand l’équipe grandit. Un code court mais incompréhensible est une victoire de dix minutes ; un code clair est une victoire de dix ans.

## Ce que Vitte n’est pas

Un langage « magique ». Un macro‑système sans garde‑fous. Un système qui cache l’ABI ou les appels externes.

Cette position peut sembler austère au début. Mais elle paie vite quand le projet grandit.

## Le rôle du compilateur

Le compilateur n’est pas un oracle. Il est un partenaire : il vous signale les incohérences, il refuse les ambiguïtés, et il vous aide à rester honnête. Cette relation est saine : elle vous pousse à expliciter vos hypothèses.

## La stabilité comme stratégie

Les projets long‑vivants demandent un langage qui ne change pas à chaque mode. La stabilité n’est pas un conservatisme, c’est une stratégie : elle réduit les coûts de maintenance et rend le code plus abordable pour les nouvelles recrues.

## Lisibilité et performance ne sont pas des ennemies

Un code lisible ne signifie pas un code lent. Au contraire : la lisibilité vous permet d’identifier les vrais chemins chauds, et donc d’optimiser là où c’est utile. Le code obscur, lui, dissipe vos efforts.

## Le contrat implicite

Vitte vous demande d’être explicite, mais en échange il promet quelque chose : un contrat stable, des diagnostics cohérents, et un pipeline compréhensible. C’est un pacte entre le langage et le développeur.

## À retenir

Le design de Vitte privilégie la durabilité du code. Si vous hésitez entre « court » et « clair », choisissez « clair ».


## Deux styles de code

Comparez ces deux styles :

Style A : court, dense, difficile à lire. Style B : plus long, mais chaque étape est visible.

Un compilateur accepte les deux. Une équipe, elle, préfère le second. La lisibilité est un acte collectif.

## Pourquoi la simplicité est une stratégie

Quand un bug survient, vous n’avez pas besoin d’une théorie. Vous avez besoin d’une trace lisible. La simplicité augmente la probabilité que votre futur vous comprenne.

## Erreurs courantes de philosophie

Confondre “minimal” et “opaque”. Penser que la syntaxe magique accélère les équipes. Sacrifier la lisibilité pour une optimisation non mesurée.

## Checklist de design

Chaque module a une responsabilité claire. Les interfaces sont petites et bien nommées. Les erreurs sont actionnables. Le build est reproductible.


## Exercice : écrire pour un autre

Prenez un petit bout de code que vous connaissez bien. Réécrivez‑le comme si vous deviez l’expliquer à une personne qui ne connaît pas votre projet. Si votre code devient plus clair, vous avez compris la philosophie.

## O’Reilly en une phrase

Un langage est une promesse d’explications futures. Vitte vous demande d’être explicite aujourd’hui pour éviter l’opacité demain.


## Code complet (API actuelle)

Exemple de deux styles : l’un compact, l’autre lisible.

```vit
proc sum_compact(a: [i32]) -> i32 {
  let i: i32 = 0
  let s: i32 = 0
  loop { if i >= a.len as i32 { break } s = s + a[i as usize]; i = i + 1 }
  give s
}

proc sum_clear(a: [i32]) -> i32 {
  let i: i32 = 0
  let s: i32 = 0
  loop {
    if i >= a.len as i32 { break }
    set s = s + a[i as usize]
    i = i + 1
  }
  give s
}
```

## API idéale (future)

Un formalisme de style (lint) qui encourage les blocs lisibles au lieu des expressions compressées.

