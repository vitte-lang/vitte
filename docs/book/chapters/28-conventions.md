# 28. Annexes : conventions

Les conventions sont le ciment des projets partagés. Elles ne remplacent pas la technique, mais elles évitent beaucoup de frictions. Un projet qui change de style à chaque fichier devient illisible. Les conventions sont un pacte implicite entre les membres d’une équipe, et ce pacte doit être assez simple pour être respecté au quotidien.

Ce chapitre propose un ensemble de règles simples, pragmatiques, et cohérentes avec le reste du livre. Elles ne sont pas “parfaites”, mais elles sont stables. La stabilité est ce qui rend un code transmissible, surtout quand l’équipe change ou que le projet traverse plusieurs années.

## 1) Nommage

### 1.1 Conventions de forme

La règle de base est simple et visible. Utilisez `snake_case` pour les fonctions et les variables, et `PascalCase` pour les `form` et les `pick`. Cette distinction permet au lecteur de reconnaître immédiatement la nature d’un symbole, sans devoir scanner l’ensemble du fichier. Une convention de forme est un gain cognitif automatique.

### 1.2 Noms d’intention vs noms mécaniques

Un nom d’intention exprime pourquoi la chose existe, pas comment elle est implémentée. Un nom mécanique décrit l’action brute, ce qui le rend fragile dès que l’implémentation change. Les noms d’intention sont donc préférables, parce qu’ils résistent aux refactors et qu’ils restent lisibles quand le code évolue.

Un nom comme `do_it` ne dit rien. Un nom comme `clamp_to_max` raconte un comportement précis, même si l’algorithme interne change. L’objectif n’est pas d’être verbeux, mais d’être fidèle à l’intention.

## 2) Imports

### 2.1 Ordre recommandé

L’ordre des imports doit être stable dans chaque fichier. Placez d’abord `space` si le fichier définit un module, puis les `use`, ensuite les `pull`, puis les déclarations (`type`, `form`, `pick`, `const`, `proc`). Cet ordre crée un en‑tête lisible et prévisible. Il permet de comprendre la “surface” du fichier avant d’entrer dans la logique.

### 2.2 Limiter les imports globaux

Importer un module entier “au cas où” crée des dépendances implicites. Préférez des imports ciblés qui rendent la dépendance visible. Un import clair aujourd’hui est une heure gagnée demain.

## 3) Modules

### 3.1 Une responsabilité par module

Un module doit faire une seule chose. C’est la règle la plus simple et la plus efficace pour garder un code lisible. Si un module a plusieurs responsabilités, il devient un point de friction et de confusion.

### 3.2 Éviter les modules “fourre‑tout”

Un module qui contient “un peu de tout” devient rapidement un lieu de dettes techniques. S’il dépasse sa responsabilité, scindez‑le tôt. Plus vous attendez, plus la scission devient coûteuse.

## 4) Tests

### 4.1 Nommage

Les tests doivent être retrouvés en quelques secondes. Utilisez des noms explicites comme `feature_name.vit`, `parser_errors.vit`, `io_read_write.vit`. Le but n’est pas l’esthétique, mais la rapidité d’accès.

### 4.2 Guide d’écriture de tests

Un test doit être court, précis, et non‑fragile. S’il échoue pour des raisons non liées à son objectif, il est mal écrit. Un test fiable est un test qui raconte une histoire simple, avec un début et une fin claire.

## 5) Erreurs et diagnostics

Les messages d’erreur doivent être courts et actionnables. Un bon message dit quoi faire, pas seulement ce qui est cassé. “error: failed” est inutile, parce qu’il ne guide aucune action. Un message comme “error: cannot open config file (missing path)” indique immédiatement ce qu’il faut vérifier.

## 6) Commits et PR

Les règles de base sont simples : des commits petits, un message clair, et aucun mélange refactor/feature. Cela rend les PR relisibles et évite les conflits. Une PR claire coûte moins cher qu’une PR parfaite.

## 7) Configuration et outillage

### 7.1 Standardiser avec un Makefile

Un Makefile explicite rend les builds reproductibles. Il doit décrire les cibles essentielles, comme `build`, `test` et `clean`. Un build explicite est plus facile à diagnostiquer qu’un script “magique”.

### 7.2 Configuration non triviale

Si votre projet a une configuration non triviale, utilisez un format stable et documenté, et décrivez‑le clairement. Un format stable évite les fichiers “magiques” et réduit le nombre de surprises en production.

### 7.3 Éviter les scripts “magiques”

Un script qui fait “trop de choses” sans documentation est un bug en attente. Si un script existe, il doit être décrit et justifié. Le coût d’une documentation courte est faible, le coût d’un script opaque est énorme.

## 8) Style

Le style n’est pas cosmétique. Il réduit les erreurs en rendant les intentions visibles. Une ligne doit exprimer une seule idée, et toute mutation doit être explicite via `set`. Ce sont des règles simples qui augmentent la clarté.

## 9) Checklist rapide

Avant de valider un fichier, vérifiez mentalement les points suivants : les noms respectent‑ils `snake_case` ou `PascalCase` selon leur nature, les imports suivent‑ils l’ordre recommandé, le module a‑t‑il une responsabilité claire, les tests sont‑ils courts et explicites, les erreurs sont‑elles actionnables, les commits sont‑ils petits et clairs, et le build est‑il documenté. Une checklist courte évite des retours coûteux.

## Exemples avant/après (noms, imports, erreurs)

Cette section illustre la différence entre une convention appliquée et une convention ignorée. Les exemples sont courts, mais ils rendent la règle tangible.

### Nommage

Avant, un nom comme `do_it` ne dit rien. Après, un nom comme `clamp_to_max` décrit une intention claire et stable. Cette différence est plus importante que la différence d’algorithme.

### Imports

Avant, des imports larges comme `use std` masquent les dépendances réelles. Après, des imports ciblés rendent chaque dépendance visible et réduisent l’ambiguïté. L’ordre `space` puis `use` puis déclarations crée un en‑tête lisible.

### Erreurs

Avant, un message générique comme “error: failed” ne dit rien. Après, “error: cannot open config file (missing path)” donne une action implicite. Un bon diagnostic est un raccourci vers la solution.

## Mini glossaire des conventions

Les noms d’intention sont des noms qui expriment le rôle, comme `parse_config`, et non des noms mécaniques comme `do_it`. Un module responsable est un module qui fait une seule chose et la fait bien. Un test non‑fragile échoue seulement quand l’intention est brisée, pas quand le format change. Un commit atomique contient une seule idée ou un seul changement logique.

## Encadré : Conventions d’équipe (adoption sans friction)

Une convention fonctionne si elle est adoptée, pas si elle est parfaite. Pour l’adoption, écrivez une règle courte, ajoutez un exemple bon/mauvais, appliquez‑la d’abord aux nouveaux fichiers, mesurez l’effet, puis étendez progressivement. L’objectif n’est pas de “forcer”, mais d’aligner.

## Conventions quand on grandit

Quand un projet grossit, les règles doivent évoluer. Ajoutez des sous‑modules quand un module dépasse une responsabilité claire, quand ses imports deviennent trop lourds, ou quand plusieurs personnes modifient régulièrement la même zone. Splitter un module devient nécessaire quand un fichier dépasse un seuil lisible, quand plusieurs sujets cohabitent, ou quand la relecture devient lente et incertaine.

## Anti‑conventions (pièges courants)

Ces pratiques semblent pratiques à court terme, mais détruisent la lisibilité. Importer un module entier “au cas où” rend les dépendances invisibles. Nommer une variable `tmp` partout rend le code incompréhensible. Mélanger refactor et feature dans un même commit rend les PR illisibles. Écrire un script magique sans documentation crée une dette immédiate. Centraliser trop tôt dans un module “utils” crée un monstre difficile à découper.

