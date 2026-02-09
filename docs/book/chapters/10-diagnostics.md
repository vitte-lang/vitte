# 10. Erreurs et diagnostics

Les diagnostics sont une partie du langage, pas un ajout. Un bon message d’erreur accélère la compréhension et évite les cycles de debugging inutiles.

## Lire un diagnostic

Prenez l’habitude de lire le code d’erreur. C’est un index stable, utile pour la recherche et la documentation. Un diagnostic est une carte : il vous indique où vous êtes et comment en sortir.

## Réduire un problème

Quand une erreur est confuse :

Isolez le cas minimal. Supprimez ce qui n’est pas nécessaire. Vérifiez que l’erreur reste.

Cette discipline accélère autant votre compréhension que celle des autres.

## Messages utiles

Un bon diagnostic répond à trois questions : où ? quoi ? pourquoi ? Le reste est secondaire.

## Des erreurs actionnables

L’erreur idéale ne se contente pas de dire « c’est faux ». Elle vous suggère une action simple : ajouter un type, corriger un module, ou renommer un identifiant. Ce n’est pas un luxe : c’est un outil de productivité.

## Le diagnostic comme documentation

Les erreurs bien formulées deviennent une documentation vivante. Elles vous apprennent la grammaire, les conventions, et les limites du langage, sans vous renvoyer systématiquement à un manuel externe.

## À retenir

Le meilleur diagnostic est celui qui vous pousse vers l’action suivante, pas celui qui vous explique toute l’histoire.


## Exemple guidé : écrire un message d’erreur

Créez un diagnostic avec :

une description courte,. le contexte,. une action suggérée.

Comparez avec un message “brut” sans action. Vous verrez la différence d’utilisabilité.

## Erreurs courantes

Messages trop longs sans action. Messages vagues (“error occurred”). Oubli du contexte (fichier, ligne).

## Checklist diagnostics

Le message dit quoi faire. Le contexte est présent. Le code d’erreur est stable.


## Exercice : un message utile

Créez un message d’erreur pour “fichier introuvable”. Comparez la version brute et une version qui inclut :

le chemin,. l’action tentée,. la suggestion (“vérifiez le chemin”).


## Code complet (API actuelle)

```vit
use std/core/result.Result

proc must_positive(x: i32) -> Result[i32, string] {
  if x <= 0 { give Result.Err("expected positive") }
  give Result.Ok(x)
}
```

## API idéale (future)

Un `Result` enrichi avec des codes d’erreur et des conseils d’action.

