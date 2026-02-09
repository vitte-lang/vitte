# 23. Projet 3 : utilitaire système (version détaillée)

Un utilitaire système est un bon test de robustesse. Il touche des fichiers, des processus, et souvent des privilèges. C’est aussi un excellent terrain pour apprendre à écrire un code qui respecte la plateforme. Ce chapitre développe le projet au maximum en paragraphes continus, sans listes, et avec un fil pédagogique clair. L’objectif n’est pas d’écrire un outil “complet”, mais un outil lisible, stable, et explicable.

## Étape 0 : définir l’intention

Avant d’écrire une ligne, définissez l’intention en une phrase. Notre outil liste un répertoire, calcule la taille totale, et affiche un résumé. Ce choix paraît simple, mais il force déjà des décisions claires : comment lire un répertoire, comment gérer les erreurs de permission, et comment formater la sortie pour être utile.

## Étape 1 : structure minimale

Un outil système doit commencer par une structure simple. Un fichier principal, quelques fonctions, et un flux clair. L’erreur classique est de mélanger l’I/O avec la logique de calcul. La correction est d’écrire une fonction qui lit, et une fonction qui calcule. Cette séparation rend le code testable et lisible.

## Étape 2 : lecture de répertoire

Lire un répertoire est une opération qui peut échouer. Un dossier peut être absent, inaccessible, ou verrouillé. La lecture doit donc produire un résultat explicite. Si l’API renvoie une erreur, votre outil doit le dire clairement. Un utilitaire système qui échoue en silence est inutile.

## Étape 3 : calcul de taille

Calculer une taille est simple sur le papier, mais dangereux si vous oubliez des cas. Par exemple, un fichier peut être un lien ou un dossier. Votre outil doit choisir une règle claire, puis l’appliquer partout. La règle la plus simple est de ne compter que les fichiers réguliers. Si vous décidez d’inclure les dossiers, documentez-le.

## Étape 4 : format d’affichage

L’affichage est une forme de contrat. Un format stable permet à l’utilisateur d’écrire des scripts autour de votre outil. Évitez les sorties ambiguës. Un format simple comme “total: X” est mieux qu’un format riche mais instable. La clarté est une fonctionnalité.

## Étape 5 : version minimale fonctionnelle

Voici un exemple minimal qui lit un répertoire, calcule une taille, puis l’affiche. Ce code est volontairement simple, et il privilégie la lisibilité.

```vit
use std/io/fs
use std/core/iter
use std/io/print
use std/core/result.Result

proc total_size(path: fs.Path) -> usize {
  let total: usize = 0
  let r = fs.read_dir(path)
  when r is Result.Err {
    let _ = eprintln("cannot read dir: " + path)
    give 0
  }
  let it = r.unwrap()
  iter.for_each(&it, proc(e: fs.DirEntry) {
    set total = total + e.metadata.size
  })
  give total
}

entry main at core/app {
  let path = "."
  let size = total_size(path)
  println_or_panic("total: " + size.to_string())
  give 0
}
```

Ce code exprime l’essentiel. Il est imparfait, mais il est clair. Une version robuste se construit toujours sur cette base.

## Étape 6 : erreurs explicites

Un utilitaire système doit être explicite sur ses erreurs. Un message court et actionnable vaut mieux qu’un message long et vague. Par exemple, “cannot read dir: /path” est plus utile que “error”. Cette discipline améliore la fiabilité de l’outil et la confiance de l’utilisateur.

## Étape 7 : variantes utiles

Une variante utile est l’ajout d’un format JSON. Cette variante force à séparer la collecte de données de la présentation. Une autre variante est l’ajout d’un filtre `--min-size`, qui oblige à valider des arguments. Ces variantes sont pédagogiques : elles montrent comment l’outil peut évoluer sans perdre la lisibilité.

## Étape 8 : tests simples et robustes

Les tests doivent couvrir trois cas : un répertoire vide, un répertoire rempli, et un cas de permission refusée. Ces trois tests suffisent à vérifier la logique principale et les erreurs. Un test qui ne couvre que le cas nominal donne une fausse confiance.

## Étape 9 : discipline de lisibilité

Un utilitaire système n’est jamais “fini”. Il évolue. La discipline qui le maintient est la lisibilité. Si votre outil devient difficile à relire, il devient fragile. La lisibilité est le vrai mécanisme de stabilité.

## Conclusion

Vous avez construit un utilitaire système simple, mais robuste. Vous avez appris à séparer l’I/O de la logique, à rendre les erreurs visibles, et à garder un format stable. Cette discipline est exactement ce qui rend un outil durable. Un utilitaire système n’a pas besoin d’être complexe pour être sérieux. Il a besoin d’être clair, stable, et prévisible.


## Étape 10 : permissions et cas refusés

Les permissions sont un vrai problème dans les outils système. Un répertoire peut être lisible mais un sous‑dossier peut être bloqué. Votre outil doit décider comment réagir : ignorer, avertir, ou échouer. La meilleure stratégie est d’avertir clairement tout en continuant, sauf si l’objectif de l’outil est strictement la précision totale. Cette décision doit être explicite dans le code et documentée.

## Étape 11 : liens symboliques et boucles

Les liens symboliques peuvent créer des cycles. Si vous parcourez un arbre en profondeur, vous risquez de boucler à l’infini. La discipline consiste à détecter ces liens et à éviter une récursion infinie. Vous pouvez choisir de les ignorer, ou de les traiter comme des fichiers, mais vous devez choisir. Un comportement implicite ici est une source de bugs critiques.

## Étape 12 : récursivité contrôlée

Un utilitaire qui parcourt des dossiers finit souvent par devenir récursif. La récursivité doit être contrôlée : profondeur maximale, stratégie d’arrêt, et gestion des erreurs par niveau. Un parcours récursif non contrôlé peut transformer un outil simple en outil dangereux.

## Étape 13 : performance et scalabilité

La performance est secondaire jusqu’à un certain point, mais un outil système peut être utilisé sur de très grands dossiers. Dans ce cas, l’important est d’éviter les allocations inutiles et de minimiser les appels système. Une stratégie simple est de traiter les entrées en streaming plutôt qu’en chargeant tout en mémoire. Ce gain est souvent plus important que des optimisations locales.

## Étape 14 : formatage et stabilité d’interface

Un format de sortie stable est une promesse pour les utilisateurs. Si votre outil change son format à chaque version, il casse les scripts. La stabilité vaut parfois plus que la beauté. C’est pourquoi il faut choisir un format simple, documenté, et durable.

