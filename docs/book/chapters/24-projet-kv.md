# 24. Projet 4 : base de données key‑value (version détaillée)

Une base key‑value est un excellent exercice : structure de données, I/O, et cohérence. C’est aussi une bonne manière d’apprendre à gérer la corruption et les formats. Ce chapitre développe le projet au maximum en paragraphes continus, en gardant la lisibilité comme objectif principal. Le but n’est pas de rivaliser avec une base industrielle, mais de construire un système simple, explicable, et robuste.

## Étape 0 : pourquoi une key‑value

Une base key‑value est la plus petite base possible. Elle ne fait qu’une chose : associer une clé à une valeur. Cette simplicité est un avantage pédagogique. Vous pouvez isoler les problèmes essentiels, comme la gestion des fichiers, la sérialisation, et la cohérence. Une base relationnelle serait trop large pour cet apprentissage. Ici, nous voulons comprendre chaque étape.

## Étape 1 : définir un format stable

Le format de stockage est le cœur de la base. Si vous changez le format, vous cassez la compatibilité. C’est pourquoi il doit être simple et stable. Un format classique consiste à écrire, pour chaque entrée, la taille de la clé, la taille de la valeur, puis les bytes. Cette structure est lisible, facile à déboguer, et suffisante pour un projet pédagogique.

Ce format a un avantage important : il est append‑only. Chaque entrée est ajoutée à la fin, ce qui rend l’écriture simple et robuste. Ce n’est pas le format le plus rapide, mais il est le plus facile à expliquer et à maintenir.

## Étape 2 : écrire une entrée

L’écriture consiste à transformer une clé et une valeur en une séquence de bytes, puis à les écrire à la fin du fichier. La robustesse vient de la discipline : chaque écriture doit être complète, chaque taille doit être valide, et chaque erreur doit être signalée. Dans un projet réel, on écrirait un checksum. Ici, vous pouvez déjà préparer cette idée.

Même si la stdlib offre un module `std/db/kv`, l’objectif est de comprendre la logique. Vous pouvez utiliser ce module pour l’I/O, mais conservez la structure mentale : écrire une entrée, vérifier le résultat, et rendre l’erreur visible.

## Étape 3 : lire une valeur

La lecture est plus coûteuse en append‑only. Vous devez parcourir le fichier et garder la dernière valeur de la clé. Ce n’est pas “optimal”, mais c’est stable et compréhensible. Dans un projet pédagogique, la lisibilité prime. L’optimisation viendra plus tard.

Pour rendre la lecture plus fiable, vous devez vérifier la cohérence du fichier. Si une taille lue dépasse la taille réelle du fichier, vous avez probablement une corruption. Dans ce cas, il faut s’arrêter et signaler l’erreur. Une lecture qui continue malgré une corruption est un bug sérieux.

## Étape 4 : index en mémoire

L’append‑only est simple mais lent. Une amélioration naturelle consiste à construire un index en mémoire : une map clé → offset. Vous parcourez le fichier une fois au démarrage et vous enregistrez la position de chaque clé. Ensuite, la lecture devient quasi instantanée. Cette étape ajoute de la complexité, mais elle est contrôlée.

La discipline ici est de séparer le “build d’index” et la “lecture”. L’index est un cache. Il doit être reconstructible. Ne le traitez jamais comme une vérité permanente.

## Étape 5 : compaction

Une base append‑only grandit à l’infini. La compaction consiste à réécrire un fichier propre qui ne contient que les dernières valeurs. Cette étape est essentielle pour éviter une explosion de taille. La compaction est aussi un exercice de rigueur : il faut copier correctement, vérifier les tailles, et remplacer l’ancien fichier de manière sûre.

Une bonne compaction est atomique : écrire dans un fichier temporaire, puis remplacer l’original. Cela évite les corruptions en cas de crash.

## Étape 6 : gestion des erreurs

Une base sans erreurs visibles est une base dangereuse. Les erreurs doivent être courtes, actionnables, et stables. Un message “db read failed” est insuffisant. Un message “db read failed: unexpected EOF” est utile. Les erreurs sont un outil de maintenance, pas un bruit.

## Étape 7 : API minimale

Une API minimale suffit pour ce projet : `open`, `get`, `put`, `delete`, `close`. Ces opérations couvrent l’essentiel. Chaque opération doit être claire et prévisible. L’API est un contrat avec l’utilisateur. Elle doit être stable.

## Étape 8 : tests et non‑régression

Les tests doivent couvrir les cas simples et les cas de corruption. Un test qui écrit une clé, relit, puis supprime est un bon début. Un test qui simule une corruption de taille est un meilleur test. Le but est de vérifier la robustesse, pas seulement la fonctionnalité nominale.

## Étape 9 : lecture et écriture via la stdlib

Si vous utilisez `std/db/kv`, vous bénéficiez d’un backend existant. Mais vous devez toujours comprendre ce qui se passe. Le module est un outil, pas un substitut de compréhension. Utilisez‑le pour simplifier le code, mais conservez l’architecture mentale du format.

```vit
use std/db/kv
use std/io/print
use std/core/option.Option
use std/core/result.Result

entry main at core/app {
  let r = kv.open("data.db")
  when r is Result.Err {
    let _ = eprintln("db open failed")
    give 1
  }
  let db = r.unwrap()
  let _ = kv.put(&db, "hello", "world")
  let g = kv.get(&db, "hello")
  when g is Result.Err {
    let _ = eprintln("db get failed")
    give 1
  }
  let opt = g.unwrap()
  when opt is Option.Some {
    println_or_panic(opt.value)
  }
  let _ = kv.close(&db)
  give 0
}
```

Ce code est minimal, mais il montre une API stable. L’important est la clarté du flux : ouvrir, écrire, lire, fermer.

## Étape 10 : persistance et sécurité

Une base key‑value est souvent utilisée pour stocker des données critiques. Même dans un projet simple, vous devez penser à la sécurité du stockage. La persistance ne se limite pas à écrire dans un fichier, elle inclut la capacité à survivre à un crash. Une stratégie simple est d’écrire des entrées avec un checksum et d’ignorer celles qui sont invalides. Ce n’est pas compliqué, mais c’est essentiel.

## Étape 11 : design et discipline

Le point central est la discipline. Une base key‑value simple peut rester fiable si vous respectez trois règles : format stable, erreurs explicites, et compaction contrôlée. Ces règles sont plus importantes que n’importe quelle optimisation. La base la plus rapide ne vaut rien si elle est incompréhensible.

## Conclusion

Vous avez construit une base key‑value simple, mais robuste. Vous avez appris à définir un format, à gérer l’I/O, à construire un index, et à compacter. Plus important encore, vous avez appris à rendre les erreurs visibles et à protéger la cohérence du stockage. Ce projet est un modèle de discipline : vous privilégiez la clarté et la stabilité, et c’est exactement ce qui rend un système utilisable à long terme.


## Étape 12 : journalisation (write‑ahead log)

La journalisation est une technique simple qui améliore la robustesse. Avant d’écrire dans le fichier principal, vous écrivez l’opération dans un journal. Si un crash arrive, vous rejouez le journal pour restaurer la cohérence. Cela peut sembler complexe, mais l’idée est simple : une opération n’est considérée “faite” que si le journal l’a enregistrée.

Dans un projet pédagogique, un WAL minimal peut être un fichier append‑only où chaque ligne représente une opération `put` ou `del`. Au démarrage, vous relisez ce fichier et vous appliquez les opérations dans l’ordre. Ensuite, vous pouvez effacer le journal. Cette technique introduit une discipline de fiabilité qui est largement utilisée dans les systèmes sérieux.

## Étape 13 : transactions (logiques)

Les transactions permettent de regrouper plusieurs opérations en un tout cohérent. Même si vous n’implémentez pas un système complet, vous pouvez simuler un “begin/commit”. L’idée est de retenir les opérations dans un buffer, puis de les écrire en bloc. Si le programme échoue avant le commit, rien n’est écrit. Cette approche réduit le risque d’état partiel.

Pour un projet simple, une transaction peut être une liste d’opérations en mémoire. Si la transaction est abandonnée, la liste est simplement ignorée. Si elle est commitée, les opérations sont écrites dans l’ordre.

## Étape 14 : snapshots

Un snapshot est une photo cohérente de la base à un instant donné. Il permet de sauvegarder un état stable, puis de revenir à cet état en cas de besoin. Dans une base append‑only, un snapshot peut être un fichier compacté complet. Il sert de point de départ pour la reconstruction.

Même si vous n’implémentez pas un système de snapshots sophistiqué, la simple capacité à générer un fichier propre est déjà un snapshot. La clé est de le rendre reproductible et documenté.

## Étape 15 : compaction incrémentale

La compaction complète peut être coûteuse sur de gros fichiers. Une compaction incrémentale consiste à déplacer une partie du fichier à la fois, ou à compacter une tranche définie. Cette technique est plus complexe, mais elle permet d’éviter des pauses longues.

Dans un projet simple, vous pouvez simuler une compaction partielle en traitant un nombre fixe d’entrées par cycle. Cette approche donne une base conceptuelle pour des systèmes plus avancés.

## Étape 16 : projets d’extension

Voici quelques extensions concrètes, chacune représentant un mini‑projet complet. L’objectif est d’apprendre en ajoutant une fonctionnalité claire et isolée.

Un premier projet consiste à ajouter des TTL (time‑to‑live). Une clé expire après un délai. Cela nécessite d’enregistrer un timestamp et d’ignorer les entrées expirées lors de la lecture.

Un deuxième projet consiste à ajouter une commande `scan` qui liste toutes les clés avec un préfixe donné. C’est utile pour explorer la base et tester l’index.

Un troisième projet consiste à ajouter un checksum par entrée. Cela vous oblige à écrire une fonction de hash et à rejeter les entrées corrompues.

Un quatrième projet consiste à ajouter un mini protocole réseau, pour accéder à la base via TCP. Cela introduit une nouvelle frontière, utile pour apprendre l’architecture système.

Ces projets sont volontairement séparés. Chacun peut être développé indépendamment, ce qui vous permet d’expérimenter sans casser l’ensemble.


## Étape 17 : WAL en pseudo‑code (concept minimal)

Ce pseudo‑code illustre un journal minimal. L’idée est d’écrire chaque opération dans un fichier de journal, puis de la rejouer au démarrage.

```vit
proc wal_append(op: string, key: string, value: string) {
  // écrire "op key value" dans wal.log
}

proc wal_replay() {
  // lire wal.log ligne par ligne
  // appliquer chaque opération dans l’ordre
}
```

Ce modèle est simple, mais il capture l’essentiel. Une opération est durable parce qu’elle est journalisée, pas parce qu’elle a été appliquée.

## Étape 18 : tests de corruption

Tester la corruption est plus utile que tester le cas nominal, parce que c’est là que les bugs détruisent des données. Une stratégie simple consiste à tronquer volontairement le fichier et à vérifier que la base refuse de lire au‑delà de ce qui est valide.

Un test de corruption peut aussi inverser quelques bytes et vérifier que le checksum échoue. Même si vous n’avez pas encore de checksum, le test vous force à réfléchir à la détection d’erreurs. Cette discipline est la différence entre un projet pédagogique et un projet robuste.


## Étape 19 : plan de tests complet

Un plan de tests complet couvre le nominal et le non‑nominal. Il ne s’agit pas d’avoir des centaines de tests, mais de couvrir les scénarios qui cassent réellement les bases key‑value.

Commencez par un test simple : écrire une clé, lire, comparer. Ajoutez ensuite un test de suppression, puis un test qui réécrit une clé avec une nouvelle valeur. Ces tests garantissent la cohérence de l’append‑only. Ensuite, testez la reconstruction de l’index après redémarrage : écrivez, fermez, rouvrez, relisez. Enfin, ajoutez un test de corruption pour vérifier que les erreurs sont détectées.

Ce plan est court, mais il couvre les risques majeurs. Un plan trop long est souvent un plan non exécuté.

## Étape 20 : benchmark simple

Le benchmarking doit rester pragmatique. Mesurez d’abord le temps d’écriture d’un lot de clés, puis le temps de lecture aléatoire. Le but n’est pas d’obtenir des chiffres absolus, mais de voir si une modification améliore ou dégrade le comportement.

Un benchmark simple consiste à écrire 10 000 entrées, puis à lire 1 000 clés aléatoires. Mesurez le temps total, puis comparez après une modification. Cette méthode donne un signal clair sans instrumentation complexe.

