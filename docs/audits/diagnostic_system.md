# Diagnostic approfondi du compilateur Vitte

Date : 2026-09-29  
Périmètre : diagnostics du compilateur C17, de la lecture du fichier au backend C.

## 1. Objectif

Un diagnostic Vitte doit répondre immédiatement à cinq questions :

1. **Quoi ?** Quelle règle a été violée ?
2. **Où ?** Quel fichier, quelle ligne, quelle colonne et quelle expression ?
3. **Pourquoi ?** Quelle information antérieure a causé l'erreur ?
4. **Comment corriger ?** Quelle modification est suggérée ?
5. **D'où vient l'erreur ?** Lexer, parser, résolution, typage, emprunt,
   génération C, compilateur C ou exécution ?

Sortie cible :

```text
error[VITTE-SEMA-0042]: type incompatible pour l'argument 1
  --> src/main.vit:8:18
   |
 8 |     add_one("42")
   |             ^^^^ attendu `int`, trouvé `string`
   |
  ::: src/math.vit:2:14
   |
 2 | proc add_one(value: int) -> int
   |              ---------- paramètre déclaré ici
   |
   = cause: l'appel exige le type déclaré par `add_one`
   = help: remplacez `"42"` par `42`
   = origin: semantic/type-check
```

## 2. État actuel constaté

### Points déjà solides

- Cinq sévérités existent : note, aide, avertissement, erreur et fatal.
- Les diagnostics ont un code stable, un message, des détails et un span.
- Le span contient fichier, offsets, lignes et colonnes de début et de fin.
- Une limite de diagnostics et un compteur de diagnostics supprimés existent.
- `warnings_as_errors`, la couleur et le masquage des codes/détails sont prévus.
- Le bag peut fusionner les diagnostics produits par plusieurs phases.
- Les textes sont copiés dans un stockage interne : pas de pointeur temporaire vers
  un message local.
- Les erreurs visibles possèdent déjà des codes par phase, par exemple
  `VITTE_PARSER_E_*`, `VITTE_SEMA_E_*` et `VITTE_IMPORT_E_*`.

### Limites majeures

1. `source/source.c` ne gère pas les sources : il expose seulement des métadonnées
   et un checksum. Il n'existe ni registre de fichiers ni index des lignes.
2. Un diagnostic ne porte qu'un seul span. Impossible d'afficher « déclaré ici »
   et « utilisé ici » dans le même diagnostic.
3. `details` est une chaîne non structurée. Elle ne distingue pas note, cause,
   aide, type attendu, type obtenu ou suggestion.
4. `show_source_line` existe dans les options, mais le renderer ne l'utilise pas.
5. La sortie montre `--> fichier:ligne:colonne`, mais pas le texte source.
6. Il n'existe pas de labels attachés aux spans (`attendu int`, `trouvé string`).
7. Il n'existe pas de chaîne de causes ou de backtrace de diagnostic.
8. Il n'existe pas d'origine structurée de phase (lexer/parser/sema/backend).
9. Il n'existe pas de suggestions éditables ou de niveau de confiance.
10. Il n'existe pas de sortie JSON/SARIF/LSP exploitable par un IDE ou la CI.
11. Les capacités fixes peuvent tronquer silencieusement code, message, détails
    et nom de fichier.
12. Le buffer de rendu de 1024 octets est inférieur à la capacité des détails
    (4096), donc un diagnostic valide peut échouer au moment de l'affichage.
13. La couleur englobe actuellement tout le diagnostic au lieu de colorer les
    éléments sémantiques ciblés.
14. `vitte_diagnostic_status()` ramène toutes les erreurs au statut `PARSE`, même
    lorsqu'elles viennent des imports, de la sémantique ou du backend.
15. Le message final `vitte: ...` duplique souvent le diagnostic structuré et
    réduit la lisibilité.

## 3. Architecture recommandée

### 3.1 Gestionnaire de sources

Créer un vrai `vitte_source_manager_t` responsable de :

- charger ou enregistrer un buffer source ;
- attribuer un `vitte_source_id_t` stable ;
- conserver chemin affiché, chemin canonique et contenu ;
- indexer le début de chaque ligne une seule fois ;
- convertir offset octet vers ligne/colonne ;
- extraire une ou plusieurs lignes sans recopier tout le fichier ;
- distinguer colonnes en octets, points de code Unicode et largeur terminal ;
- normaliser CRLF/LF sans perdre les offsets originaux ;
- gérer sources virtuelles, stdin, code généré et modules importés ;
- conserver la relation entre source générée et source Vitte d'origine.

Le span devrait référencer un identifiant plutôt qu'un `const char *` :

```c
typedef uint32_t vitte_source_id_t;

typedef struct vitte_span {
    vitte_source_id_t source_id;
    size_t start_offset; /* inclusif */
    size_t end_offset;   /* exclusif */
} vitte_span_t;
```

La ligne et la colonne deviennent des données calculées. Cela évite les spans
incohérents contenant à la fois offsets et positions pré-calculées divergentes.

### 3.2 Diagnostic structuré

Un diagnostic doit contenir :

- identifiant numérique ou symbolique stable ;
- sévérité ;
- phase d'origine ;
- message principal court ;
- span principal ;
- liste de labels primaires et secondaires ;
- liste de notes ;
- liste d'aides ;
- liste de suggestions ;
- chaîne de causes ;
- identifiant du diagnostic parent éventuel ;
- état « erreur utilisateur » ou « erreur interne du compilateur ».

Types proposés :

```c
typedef enum vitte_diagnostic_origin {
    VITTE_ORIGIN_IO,
    VITTE_ORIGIN_LEXER,
    VITTE_ORIGIN_PARSER,
    VITTE_ORIGIN_IMPORT,
    VITTE_ORIGIN_NAME_RESOLUTION,
    VITTE_ORIGIN_TYPE_CHECK,
    VITTE_ORIGIN_CONSTANT_EVAL,
    VITTE_ORIGIN_HIR,
    VITTE_ORIGIN_IR,
    VITTE_ORIGIN_C17_BACKEND,
    VITTE_ORIGIN_C_COMPILER,
    VITTE_ORIGIN_LINKER,
    VITTE_ORIGIN_RUNTIME,
    VITTE_ORIGIN_INTERNAL
} vitte_diagnostic_origin_t;

typedef enum vitte_label_style {
    VITTE_LABEL_PRIMARY,
    VITTE_LABEL_SECONDARY
} vitte_label_style_t;

typedef struct vitte_diagnostic_label {
    vitte_span_t span;
    vitte_label_style_t style;
    const char *message;
} vitte_diagnostic_label_t;
```

### 3.3 Suggestions applicables

Chaque suggestion doit porter :

- le span à remplacer ;
- le texte de remplacement ;
- un message ;
- une applicabilité : certaine, probable, à vérifier, non automatique ;
- éventuellement plusieurs modifications atomiques pour une seule correction.

Cela permet ultérieurement `vitte fix`, l'intégration LSP et les correctifs IDE.

### 3.4 Chaîne d'origine

Pour savoir « d'où vient l'erreur », enregistrer des frames structurées :

```text
type-check: argument incompatible dans `main`
  caused by declaration: paramètre `value: int` dans `math.vit`
  reached through import: `use math.add_one` dans `main.vit`
```

Frames utiles :

- import demandé ici / module déclaré ici ;
- symbole utilisé ici / symbole déclaré ici ;
- type attendu ici / type produit ici ;
- contrainte créée ici / contrainte violée ici ;
- macro invoquée ici / fragment généré ici ;
- générique instancié ici / paramètre contraint ici ;
- code Vitte ici / code C généré ici / erreur du compilateur C ici.

La chaîne doit être bornée et détecter les cycles.

## 4. Rendu terminal de niveau Rust/Clang

Le renderer devrait :

1. afficher code, sévérité et message sur une ligne ;
2. afficher le chemin relatif au projet par défaut ;
3. afficher 1 à 2 lignes de contexte avant et après ;
4. numéroter les lignes avec une largeur dynamique ;
5. dessiner `^` pour le primaire et `-` pour les secondaires ;
6. placer les labels sans chevauchement illisible ;
7. gérer les spans multilignes avec début, continuation et fin ;
8. raccourcir les lignes très longues avec ellipses ;
9. respecter tabulations, Unicode et largeur d'affichage ;
10. désactiver automatiquement la couleur si stderr n'est pas un TTY ;
11. respecter `NO_COLOR`, `CLICOLOR` et `--color=auto|always|never` ;
12. regrouper notes et aides sous le diagnostic principal ;
13. afficher une suggestion sous forme de mini-diff ;
14. limiter la répétition de fichiers identiques ;
15. terminer par un résumé singulier/pluriel correct.

## 5. Diagnostics attendus par phase

### Lecture et imports

- fichier introuvable avec chemin demandé et répertoires recherchés ;
- permission refusée distincte de « introuvable » ;
- encodage invalide avec offset exact ;
- import circulaire avec cycle complet `a -> b -> c -> a` ;
- module déclaré différent du module demandé ;
- symbole privé avec déclaration secondaire ;
- collision d'import avec les deux origines ;
- suggestion du nom le plus proche ;
- erreur dans un module importé avec trace depuis l'import racine.

### Lexer

- caractère inconnu souligné ;
- séquence UTF-8 invalide ;
- chaîne non terminée du guillemet à EOF ;
- échappement invalide avec liste des échappements autorisés ;
- nombre invalide ou débordant avec suffixe concerné ;
- commentaire bloc non terminé avec début du commentaire ;
- tabulation/indentation diagnostiquée seulement si la grammaire le requiert.

### Parser

- token trouvé et ensemble réduit de tokens attendus ;
- détection d'un délimiteur manquant avec insertion suggérée ;
- parenthèse/accolade fermante reliée à l'ouvrante ;
- récupération synchronisée pour éviter les erreurs en cascade ;
- diagnostic spécifique avant le générique ;
- suppression des doublons au même offset ;
- contexte syntaxique : « pendant l'analyse des paramètres de `foo` » ;
- distinction syntaxe non prise en charge / syntaxe invalide.

### Résolution et sémantique

- nom inconnu avec candidats proches et imports possibles ;
- double déclaration montrant les deux déclarations ;
- symbole privé montrant sa visibilité ;
- mauvais nombre d'arguments montrant signature et appel ;
- incompatibilité de type avec attendu/trouvé structurés ;
- opérateur invalide avec types gauche/droite ;
- affectation immuable avec lieu de déclaration ;
- retour absent ou type de retour incorrect ;
- branche non exhaustive avec cas manquants ;
- code inaccessible en avertissement ;
- variable inutilisée avec préfixe `_` suggéré ;
- contrainte générique non satisfaite avec chaîne de contraintes ;
- erreur constante reliée à l'expression d'origine ;
- diagnostic racine conservé, diagnostics dérivés supprimés.

### HIR, IR et backend C17

- toute invariance HIR/IR cassée devient une erreur interne, pas utilisateur ;
- joindre phase, fonction, bloc et instruction concernée ;
- générer un identifiant de rapport interne ;
- conserver une source map Vitte → C ;
- intercepter stderr du compilateur C ;
- parser les diagnostics Clang/GCC lorsque possible ;
- remapper les positions C générées vers les spans Vitte ;
- conserver le diagnostic C brut sous `--verbose` ;
- distinguer échec de génération, compilation et édition de liens ;
- afficher la commande externe seulement sous `--verbose` ou en erreur interne.

### Exécution

- associer panic/assertion à un span Vitte ;
- stack trace avec noms Vitte, fichiers et lignes ;
- cause (`division par zéro`, bounds, null, contrat) structurée ;
- frames internes masquées par défaut mais visibles avec `--backtrace=full` ;
- codes de sortie documentés et stables.

## 6. Formats machine

Ajouter une abstraction de renderer :

- `human` : terminal riche ;
- `short` : une ligne `fichier:ligne:colonne: severity[code]: message` ;
- `json` : schéma versionné et une entrée par ligne ;
- `sarif` : CI et analyse de code ;
- `lsp` : conversion vers `PublishDiagnostics` et `CodeAction`.

Le modèle en mémoire doit rester indépendant du format de sortie.

## 7. Qualité des messages

Règles rédactionnelles proposées :

- message principal court, sans point final ;
- dire le problème, pas seulement « erreur de syntaxe » ;
- utiliser le vocabulaire Vitte de la documentation ;
- afficher les identifiants et types entre backticks ;
- ne jamais exposer une enum interne au développeur Vitte ;
- une cause racine par diagnostic principal ;
- transformer les conséquences en notes ou les supprimer ;
- codes stables, recherchables et documentés ;
- chaque code possède explication longue et exemples correct/incorrect ;
- ne pas proposer un correctif si sa validité n'est pas suffisamment certaine.

Convention suggérée : `VITTE-<PHASE>-<NNNN>`, par exemple
`VITTE-PARSE-0007`, `VITTE-TYPE-0042`, `VITTE-IMPORT-0011`.

## 8. Robustesse et sécurité

- valider `start <= end <= source_length` à la création du span ;
- rendre les spans demi-ouverts de façon uniforme ;
- ne jamais indexer un buffer avec une colonne d'affichage ;
- ne pas couper au milieu d'un caractère UTF-8 ;
- échapper les caractères de contrôle dans chemins et extraits ;
- borner lignes, labels, causes et volume total rendu ;
- gérer proprement l'allocation impossible lors d'une erreur ;
- réserver un chemin minimal sans allocation pour OOM et fatal interne ;
- ne pas utiliser le texte utilisateur comme chaîne de format ;
- éviter toute perte silencieuse : exposer explicitement la troncature ;
- garantir que produire un diagnostic ne provoque jamais un second crash.

## 9. Tests nécessaires

### Unitaires

- conversion offset ↔ ligne/colonne ;
- LF, CRLF, dernière ligne sans newline ;
- ASCII, accents, CJK, emoji, combinants et tabulations ;
- spans vides, un caractère, multilignes et fin de fichier ;
- labels chevauchants ;
- troncature de ligne et de message ;
- couleur auto/always/never ;
- limite de diagnostics ;
- sérialisation JSON et échappement ;
- source disparue ou virtuelle.

### Golden/snapshots

- un fichier attendu par code de diagnostic ;
- sorties sans couleur stables ;
- sorties couleur testées séparément ;
- diagnostics multi-fichiers ;
- suggestions et mini-diffs ;
- trace d'import et remapping backend C.

### Fuzzing

- lexer/parser sur octets arbitraires ;
- renderer sur spans et labels arbitraires ;
- UTF-8 invalide ;
- JSON toujours valide ;
- aucun crash, dépassement, boucle ou lecture hors limites sous ASan/UBSan.

## 10. Priorités d'implémentation

### P0 — origine visible et fiable

1. Remplacer le faux module `source` par un source manager réel.
2. Unifier les spans autour de `source_id + [start,end)`.
3. Afficher extrait, numéros de ligne et underline primaire.
4. Activer réellement `show_source_line`.
5. Corriger le buffer de rendu fixe et signaler toute troncature.
6. Ajouter l'origine de phase au diagnostic.
7. Ne plus convertir toute erreur en statut `PARSE`.

### P1 — explication de la cause

8. Ajouter plusieurs labels/spans.
9. Ajouter notes et aides structurées.
10. Montrer déclaration et utilisation ensemble.
11. Ajouter attendu/trouvé structurés pour le typage.
12. Ajouter traces d'import et cycles.
13. Dédupliquer et limiter les cascades.
14. Ajouter des tests golden pour les diagnostics existants.

### P2 — correction et outils

15. Ajouter suggestions et applicabilité.
16. Ajouter rendus `short` et JSON versionné.
17. Exposer diagnostics et code actions au LSP.
18. Documenter chaque code d'erreur.
19. Ajouter `--explain CODE`.
20. Ajouter `vitte fix` uniquement pour suggestions certaines.

### P3 — backend et exécution

21. Produire des source maps Vitte → C.
22. Remapper les erreurs Clang/GCC.
23. Ajouter stack traces Vitte au runtime.
24. Ajouter panic spans et chaînes de causes.
25. Ajouter SARIF pour CI.

## 11. Critère de réussite

Le système est prêt lorsque, pour chaque erreur utilisateur courante :

- le bon fragment de source est souligné ;
- la déclaration ou origine pertinente est montrée ;
- la phase fautive est identifiable ;
- la cause racine est séparée des erreurs en cascade ;
- une aide sûre est fournie quand elle existe ;
- la même information est disponible en terminal, JSON et LSP ;
- un diagnostic invalide ou extrême ne peut pas faire planter le compilateur.

## Conclusion

Vitte possède déjà le début d'un modèle de diagnostic, mais pas encore
l'infrastructure source nécessaire pour atteindre le niveau de Rust ou Clang.
Le meilleur premier jalon n'est pas d'ajouter davantage de chaînes `details` :
il faut construire le source manager, normaliser les spans, puis permettre les
labels multi-spans. Ces trois éléments rendent enfin visible **où**, **pourquoi**
et **d'où** vient chaque erreur.
