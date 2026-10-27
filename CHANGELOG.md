# Changelog

## [Unreleased]

### État de la voie native du compilateur

- les opérations de labels de diagnostic sont portées en Vitte; seules leurs
  deux fonctions `dump` vers flux hôte gardent une frontière externe ;
- le registre des diagnostics (149 entrées) et ses recherches, validations,
  compteurs et empreintes sont portés en Vitte ;
- la source map possède ses entrées dans un vecteur Vitte et porte les
  recherches, remappages, traces, statistiques et empreintes en natif ;
- les valeurs de schéma, sévérités et options par défaut des renderers JSON,
  terminal, SARIF et LSP sont natives ; leurs écrivains C gardent l'ABI bootstrap
  et exigent une conversion de layout ou un serializer Vitte avant leur
  branchement aux diagnostics natifs ;
- les noms et validateurs du pont CLI, la validation par lot et ses compteurs
  sont portés en Vitte ; l'explication et les rendus vers flux restent hôtes ;
- les suggestions et plans de correction possèdent leurs spans et chaînes de
  remplacement en Vitte ; le convertisseur de positions LSP natif couvre les
  unités UTF-8, UTF-16 et UTF-32 à partir du `SourceContext` natif ;
- l'audit courant compte 271 bindings `export extern proc` dans 17 modules du
  compilateur, dont 28 dans six modules de diagnostic ; cette mesure remplace
  les comptes précédents de 268 au total et 25 pour les diagnostics ;
- le build standard lie le pipeline Vitte pour `compile`, `run` et `--emit-c` ;
  le frontend natif lex, parse, analyse et émet le C17 depuis l'AST avant de
  rendre la main au driver hôte C pour l'objet, le lien et l'exécution ;
- l'API Vitte conserve le C généré et ses diagnostics dans `ApiResult` ;
- les erreurs codegen natives sont converties en diagnostics CLI structurés
  avec code backend, origine, détails et statut `ErrorBackend`, regroupés avec
  les diagnostics du frontend ; le CLI ne retente plus `compile`/`run` via
  bootstrap après un échec natif sans diagnostic et signale une erreur de
  phase. `check`, inspection et `--emit-ir` gardent leurs étapes bootstrap ;
- le parcours reste incomplet : HIR/IR natifs ne pilotent pas cette route,
  l'orchestration de compilation et d'édition de liens reste en C, 271 bindings
  externes subsistent et aucun cycle propre d'auto-compilation n'est validé ;
- le contrôle sémantique de `pipeline.vitte` et la compilation C17 du pilote
  hôte passent, mais `make native-frontend` reste bloqué par E0902 lors de
  l'émission de `api.vitte` ;
- aucune preuve de parité complète ni de cycle autonome reconstruction puis
  exécution n'est disponible ; le retrait des chemins bootstrap de `check`,
  `--emit-ir` et des modules C reste ouvert jusqu'à ce que ces gates passent.

### Frontière des intrinsics

- inventaire des contrats atomiques, mémoire volatile, allocation runtime,
  traps et conversions de représentation, avec leur statut réel dans le
  registre, l'IR et les backends;
- définition des règles d'abaissement direct et des diagnostics requis pour
  exclure les imports C des APIs Vitte concernées.

### Contrats de cibles

- vérification des largeurs de pointeurs, des modèles ABI et des prérequis
  atomiques pour x86_64, AArch64, RISC-V 64 et WebAssembly 32;
- relevé des incohérences de validation entre codegen, backend C17 et `core`,
  ainsi que de l'absence de layout cross-compilé et de capacités CPU atomiques.

### Bibliothèque `alloc` branchée sur `core`

- ajout des allocateurs système, zéro, nul et tampon personnalisé, ainsi que
  des adaptateurs fallback, composite, limite, comptage et suivi;
- ajout des façades de layouts et d'opérations mémoire qui délèguent à
  `core::mem` et `core::intrinsics::memory`;
- documentation des contrats de libération, des réservations de taille nulle
  et des blocages runtime qui empêchent encore les pools concurrents, les
  hazard pointers, la réclamation par epochs et le TLS natif.

### Port progressif du frontend

- conversion HIR des expressions d'indexation et d'accès membre, des types
  slice et des affectations composées ; rejet des cibles
  d'affectation invalides avant la validation HIR ;
- abaissement IR des indexations en GEP/LOAD/STORE, prise en charge des
  affectations composées sur une cible indexée et conversion des tranches en
  pointeurs d'éléments pour l'IR ;
- abaissement IR des propriétés de longueur et des opérations `len()` et
  `slice()` sur les chaînes, avec émissions C17 correspondantes ; alias HIR
  `str` et type `usize` alignés sur leurs représentations de chaîne et `u64` ;
- représentation IR des tranches `[T]` sous forme de `{data, length}`, accès
  à la longueur pour `len()`/`length`, et indexation via le pointeur de données ;
- création de symboles HIR distincts pour les paramètres et variables locales,
  et contextualisation IR des littéraux entiers et `null` dans les comparaisons ;
- abaissement récursif des branches `else if`, nécessaire aux fonctions du
  module mémoire ;
- typage HIR des concaténations de chaînes et opération IR `string_concat`,
  émise en C17 avec allocation et vérification du dépassement de capacité ;
- inférence du type HIR des variables locales initialisées sans annotation ;
- acceptation d’un `return` terminal sans point-virgule avant `}` ou la fin du
  fichier ; résolution de `i8` et `i16` selon le modèle signé élargi du bootstrap,
  permettant l’émission IR/C17 de `math.basic.max` ;
- prise en charge de `&&` et `||` dans le parser, la sémantique et le backend C17 ;
  ajout des types numériques manquants à l’émission C17 et contextualisation des
  littéraux flottants dans les appels IR, permettant la compilation de
  `math.basic.clamp` ;
- contextualisation d’un littéral flottant vers `f32` dans une opération mixte
  avec un opérande `f32`, sans conversion implicite des valeurs `f64` ;
  `math.basic.arithmetic` compile jusqu’à l’objet C17 ;
- migration des liaisons locales historiques `value` vers `let` avec leur
  terminaison `;` requise par la grammaire ; `math.basic.sign` compile jusqu’à
  l’objet C17 ;
- migration des 32 liaisons historiques de `math.basic.mod` vers `let`, avec
  les bindings des boucles de multiplication et d’exponentiation marqués `mut` ;
  le module compile jusqu’à l’objet C17 ;
- implémentation de `fpclassify_f32` et `fpclassify_f64` avec codes publics
  stables pour NaN, infini, zéro, sous-normal et normal ;
- implémentation de `isfinite_f32` et `isfinite_f64` : rejet de NaN par
  auto-comparaison et des infinis par comparaison aux bornes finies IEEE 754 ;
- implémentation de `isinf_f32` et `isinf_f64` : détection des infinis positifs
  et négatifs par dépassement des bornes finies, sans confondre NaN ;
- implémentation de `isnan_f32` et `isnan_f64` par auto-comparaison IEEE 754 ;
- implémentation de `isnormal_f32` et `isnormal_f64` : acceptation des valeurs
  normales signées et rejet de zéro, des sous-normaux, NaN et infinis ;
- implémentation de `signbit_f32` et `signbit_f64` par lecture du bit IEEE 754
  dans la représentation en octets, préservant le signe de zéro et des NaN ;
- implémentation de `isgreaterequal_f32` et `isgreaterequal_f64` avec comparaison
  ordonnée `>=`, qui retourne faux si l’un des opérandes est NaN ;
- prise en charge du déréférencement de pointeurs en HIR et des lectures/
  écritures correspondantes en IR (`LOAD`/`STORE`) ; ajout des paramètres des
  déclarations externes à l’IR pour permettre l’émission C17 des signatures FFI ;
- ajout d'une cible `make native-frontend` séparée du compilateur par défaut ;
- exécution du gestionnaire de sources et du lexer en Vitte, avec comparaison
  de la sortie du lexer bootstrap ;
- raccordement progressif du parser, de l'analyse sémantique, du lowering HIR,
  du lowering IR et de l'orchestrateur codegen bootstrap ;
- prise en charge des branches `elif` dans les instructions conditionnelles du
  parser bootstrap ;
- reconnaissance lexicale et prise en charge de `sizeof` avec opérande expression
  ou type dans le parser, l'analyse sémantique et l'émission C17 bootstrap ;
- prise en charge des types de tableau fixes `[T; N]` dans le parser bootstrap,
  avec l'ordre longueur puis élément attendu par la sémantique et le backend ;
- association des diagnostics des modules assemblés à leur fichier et à leurs
  offsets sources d'origine dans `check`, y compris pour le texte extrait du
  span d'un diagnostic de nom inconnu ; les diagnostics génériques de phase
  réutilisent aussi l'origine quand leur span reste dans un seul fichier ;
- correction de la sémantique bootstrap pour `i32`, `u16`, `usize.MAX`, les
  opérations intégrées sur les chaînes, les vues de tableaux en tranches, les
  conversions de pointeurs et les appels via pointeurs de fonctions ;
- correction des caractères de validation de version et du hachage des chaînes
  pour respecter l'indexation en `char` et les octets UTF-8 ;
- parenthésage de l'adresse du témoin d'endianness avant sa conversion en
  pointeur d'octets ;
- autorisation des conversions explicites entre pointeurs et entiers pour les
  calculs d'adresses bas niveau ;
- prise en charge bootstrap de `alignof(type)` dans le lexer, le parser,
  l'analyse sémantique, le lowering HIR et l'émission C17 ;
- remplacement des casts par appel de type dans les constantes de l'arène par
  `as`, et calcul des bornes avec `usize.MAX` ;
- alignement par défaut de l'arène calculé avec `alignof(usize)` ; les demandes
  d'alignement supérieur passent par les paramètres dédiés d'allocation ;
- remplacement des retours `?T` de `arena/block.vitte` par des indicateurs de
  réussite et des paramètres de sortie pour les valeurs ;
- représentation des types optionnels distincte des pointeurs dans la sémantique,
  avec extraction de la valeur par `?` et émission C17 adaptée aux options de
  valeurs, de références et de pointeurs ;
- correction des bornes `usize.MAX` et du calcul d'alignement de
  `arena/allocator.vitte`, avec résultat booléen et paramètre de sortie au lieu
  d'un pointeur vers une valeur locale ;
- correction des bindings mutables de l'allocateur, de la référence mutable
  conservée dans `ArenaMark` et de l'accès au ratio défini par le module `block` ;
- alignement des espaces de noms de l'arène, de l'allocateur et des blocs sur
  leurs chemins `compiler::arena::*`, avec import sélectif des symboles utilisés ;
- publication des types et opérations de `ArenaContext` consommés par les autres
  modules du compilateur ;
- correction des bornes `usize.MAX`, de la mutabilité de la configuration et des
  compteurs, et passage explicite de références mutables aux opérations d'arène ;
- correction de `arena/checkpoint.vitte` : espace de noms et imports, initialisation
  complète de `ArenaMark`, et déballage explicite des références optionnelles après
  leurs contrôles de nullité ;
- correction de `arena/reset.vitte` : imports et callbacks typés, déréférencement
  contrôlé des arènes et pools, namespace du module `pool`, initialisation des
  buffers et indexation compatibles avec le bootstrap ;
- correction de `arena/stats.vitte` : imports, accès par références, mutations via
  références mutables et deltas signés ; résolution bootstrap de `isize` vers le
  modèle arithmétique `i64` ;
- correction de `ast/ast.vitte` : imports d'arène, bornes `usize`, types des
  drapeaux et des helpers récursifs, et renommage du paramètre réservé `self` ;
- correction de `backend/c17/ast_emitter.vitte` : renommage des identifiants
  réservés, références AST déballées après validation, bras `match` conformes
  au parseur et typage explicite des littéraux ; adaptation de `writer.vitte`
  et `lexer.vitte` aux signatures et types acceptés par l'analyse sémantique ;
- correction de `backend/c17/ir_emitter.vitte` : syntaxe des bras `match`,
  encodage des caractères hexadécimaux et affectations du nom de fonction ;
- correction de `backend/c17/module.vitte` : types de tableaux dynamiques,
  contrats optionnels des chemins et noms, et comparaison lexicographique des
  chaînes pour le tri déterministe ;
- correction de `backend/c17/options.vitte` : terminaison des branches `match`
  avec la syntaxe attendue par le parseur ;
- correction de `backend/c17/program.vitte` : tableaux dynamiques, chaînes non
  nullables, bornes `u64`, tri lexicographique et initialisation des compteurs ;
- correction de `backend/c17/translation_unit.vitte` : tableaux dynamiques,
  noms non nullables, références au programme et au backend, et tri déterministe
  des sections et chaînes ;
- correction de `backend/c17/naming.vitte` : identifiants réservés, contrats de
  chaînes non nullables, bornes `u64`, cast des octets hexadécimaux et tableaux
  de compteurs ;
- correction de `builtin/builtin.vitte` : suffixe numérique du hachage, bornes
  `u64`, traitement des noms non nullables et déplacement du parcours par
  callback auprès des accesseurs du registre pour garder son type vérifiable ;
- déballage explicite des détails optionnels dans `api/error.vitte` avant leur
  normalisation en chaîne bornée ;
- correction de `source/source.vitte` : mutations via références déballées,
  contrôles de chaînes non nullables, accès explicite aux entrées optionnelles
  et typage des littéraux de capacité ;
- correction de `diagnostic/diagnostic.vitte` : renommage du binding réservé
  `diagnostic`, références et pointeurs de spans adaptés à la syntaxe actuelle,
  et passage explicite des références mutables aux fonctions de normalisation ;
- prise en charge de `diagnostic` comme segment de chemin réservé dans le
  parseur et la résolution sémantique bootstrap, pour charger les modules du
  sous-système de diagnostics ;
- correction de `diagnostic/engine.vitte` : références modernes, renommage des
  bindings réservés et mise à jour d’une copie du diagnostic suivie de son
  enregistrement dans le sac ;
- renommage des paramètres réservés dans `diagnostic/label.vitte` ;
- renommage des paramètres réservés dans `diagnostic/suggestion.vitte` ;
- correction de `diagnostic/cascade.vitte` : renommage des bindings réservés et
  modifications du sac par copie locale réassignée, avec emprunts mutables
  explicites lors du calcul des cascades ;
- renommage des paramètres réservés `diagnostic` et `entry` dans
  `diagnostic/registry.vitte` ;
- renommage des paramètres réservés `entry` dans `diagnostic/source_map.vitte` ;
- renommage du paramètre réservé `diagnostic` dans `diagnostic/render_json.vitte` ;
- renommage du paramètre réservé `diagnostic` dans `diagnostic/render_lsp.vitte` ;
- renommage du paramètre réservé `diagnostic` dans `diagnostic/render_sarif.vitte` ;
- renommage du paramètre réservé `diagnostic` dans `diagnostic/render_terminal.vitte` ;
- renommage des paramètres réservés dans `diagnostic/cli_bridge.vitte` ;
- correction de `diagnostic/explain.vitte` et `diagnostic/help.vitte` :
  renommage des bindings réservés `entry` et `diagnostic`, et déballage correct
  des pointeurs de registre après vérification ;
- correction de `api/context.vitte` : imports sélectifs, chaînes optionnelles,
  références mutables et sorties explicites pour les sources et le sac de
  diagnostics ;
- renommage des identifiants réservés dans `codegen/codegen.vitte` ;
- correction des identifiants réservés dans `driver/driver.vitte` ;
- déballage explicite de la source optionnelle dans `driver/pipeline.vitte` ;
- correction de `api/api.vitte` : imports sélectifs, ABI externe, tampons d’octets,
  références mutables et déballage explicite des entrées optionnelles ;
- correction de `cli/cli.vitte` : masques d’options typés, chaînes optionnelles,
  références aux parsers d’options et hachage explicite des booléens ;
- renommage du paramètre réservé dans `printer/printer.vitte` ;
- renommage des paramètres `block` qui masquaient le module importé dans
  `arena/allocator.vitte` ;
- complétion des signatures de procédure pour vérifier les callbacks et
  résolution des erreurs sémantiques de `backend.vitte` ;
- analyse sémantique sans diagnostic des 58 modules Vitte de `src/compiler`,
  avec la racine standard `modules/core/src` configurée ;
- correction des bindings immuables utilisés comme accumulateurs dans
  `compiler/api/version.vitte` et mutation explicite du contenu via `&mut` ;
- ajout des erreurs, phases, comptes et empreintes HIR/IR au résultat du
  pipeline ;
- génération C17 via le callback codegen et l'émetteur IR bootstrap ; la sortie
  temporaire est mesurée puis supprimée, car l'API n'expose pas encore de cible
  de sortie ;
- abaissement C17 des appels `.len()` sur chaînes et tableaux ou tranches
  dynamiques dans le frontend natif, avec accès correct aux références du lexer ;
- suppression des avertissements C du build `native-frontend` courant, dont les
  helpers runtime inutilisés et la chaîne source trop longue de l'émetteur ;
- implémentation native du cycle de vie du parser Vitte : initialisation,
  remise à zéro, destruction, validation du contexte et réglage des limites ;
- port Vitte du curseur de tokens du parser, des recherches token/nœud-token et
  des accesseurs de diagnostics, avec arrêt à EOF et protection du lookahead
  contre les débordements ;
- port du stockage plat des nœuds du parser et de leurs listes d’enfants, avec
  identifiants séquentiels conservés lors de la croissance, buffers vérifiés et compteurs
  d’allocations saturants ; accesseurs de nœuds et de racine en Vitte ;
- port de l’émission et du stockage des diagnostics du parser, avec spans dérivés
  du token courant, limites configurées et suppression des erreurs répétées
  pendant la récupération ;
- port des primitives de récupération du parser : suivi des délimiteurs imbriqués,
  synchronisation des déclarations, instructions et listes, points d’échappement
  et respect de la limite de tokens de récupération ;
- premières productions grammaticales du parser natif : nœuds d’identifiants et
  de chemins, mots-clés contextuels, segments réservés de modules, séparateurs
  `::` et `/`, et spans AST couvrant les chemins ;
- port de la déclaration `use` : chemins simples, alias, import glob et listes
  d’éléments aliasés, avec synchronisation à la virgule ou à `}` et drapeau AST
  d’import glob ;
- port de la clause `export` : chemin simple, alias, export global et groupes
  d’éléments aliasés, avec récupération de liste et drapeau AST public ;
- première tranche des déclarations `const` et `static` : drapeaux public et
  mutable, annotations par expressions de type et initialiseurs atomiques ;
- port du parseur d’expressions unaires et binaires : parenthèses, opérateurs
  et priorités/associativités du bootstrap, avec spans et indices de tokens ;
- port des postfixes d’expression : appels avec arguments et récupération de
  liste, indexations, accès membres et propagation postfixée `?`, avec spans
  étendus sur l’expression complète ;
- port des casts `as` après les expressions postfixées, avec nœud AST `CastExpr`
  et span couvrant l’expression et le type cible ;
- port des expressions de type : procédures, types optionnels, références,
  pointeurs qualifiés, tableaux/slices et arguments génériques avec récupération
  de liste ; les casts et annotations `const`/`static` utilisent cette grammaire ;
- port des constructeurs de formes, y compris génériques `Type<T> { ... }`, avec
  champs `FieldInit`, récupération des erreurs de liste et span couvrant la forme ;
- port de formes primaires du bootstrap : `self`, littéraux de tableau, `print`,
  `sizeof(expr/type)` et `alignof(type)`, avec drapeau de requête de type et spans ;
- port des expressions `if`/`elif`/`else` et des blocs expressions à valeur unique,
  avec diagnostic si `else` manque et spans couvrant chaque forme ;
- port des blocs d’instructions avec bindings `let` simples, expressions et
  affectations `set`, spans complets et synchronisation des erreurs par instruction ;
- ajout des instructions `return`, `break` et `continue` aux blocs, avec valeur
  de retour facultative et point-virgule obligatoire ;
- port des instructions conditionnelles `if`/`elif`/`else` avec blocs ordinaires,
  chaînage des branches et spans étendus ;
- ajout des instructions `while` avec condition, corps `Block` et span couvrant
  toute la boucle ;
- ajout des boucles inconditionnelles `loop` avec corps en bloc et span complet ;
- port de la boucle `for` avec binding simple, expression itérable et corps `Block` ;
- extension du parseur shadow natif aux déclarations `type`, `opaque`, `form`,
  `pick`, `trait`, `impl`, `proc`, `extern proc`, `macro` et `test` ;
- ajout des paramètres génériques bornés et des clauses `where` aux déclarations
  natives de types, formes, picks, procédures, traits et implémentations ;
- prise en charge syntaxique des préfixes `#[...]` et `#![...]`, avec propagation
  du drapeau `HAS_ATTRIBUTES` à l’AST natif sans conserver noms ni valeurs ;
- prise en charge des contrats `requires` et `ensures` dans les procédures et
  procédures externes, avec expressions terminées par `;` et type de contrat
  conservé sur la racine de chaque condition AST ;
- correction de l’émission C17 des alias de types et distinction des pointeurs
  bruts des références ; `parser.vitte` et son runtime mémoire génèrent et
  compilent maintenant sans avertissement avec les options strictes du projet ;
- initialisation sûre du nœud module HIR dans le chemin d'échec du lowerer ;
- maintien du compilateur par défaut sur le bootstrap C17 jusqu'au port natif
  du parser, de l'analyse sémantique, de HIR, d'IR et de codegen, puis au
  raccordement de l'écriture de sortie et de la compilation native.

## [0.1.1] — 2026-10-04

Cette version regroupe les changements réalisés depuis la 0.1.0 du
28 septembre 2026. Comparaison de base : `edc67b6`.

### Résumé

- nouveau bootstrap natif C17 et pipeline frontend exploitable depuis la CLI ;
- grammaire, lexer, parser, analyse sémantique et diagnostics synchronisés ;
- graphe d'import avec cache, cycles, alias, visibilité et origines ;
- sorties de diagnostics terminal, JSON, SARIF et LSP unifiées ;
- contrôles de types plus stricts et conversions perdantes explicites ;
- module JSON renforcé et couvert par des tests de limites ;
- nouveaux tests de régression, fuzzing, ASan/UBSan et compilations concurrentes ;
- installateurs, paquets, CI, documentation et intégrations éditeur actualisés.

### Compilateur et bootstrap

- migration du point d'entrée et des sous-systèmes vers `src/bootstrap` ;
- découpage du driver en CLI, configuration, pipeline, diagnostics et exécution ;
- ajout de contrats publics C17 pour l'API, le lexer, le parser, les sources,
  les imports, les symboles, les types, le HIR, l'IR et le backend C17 ;
- ajout de Makefiles autonomes pour les sous-systèmes ;
- fusion des options CLI et configuration avec validation centralisée ;
- validations d'invariants, empreintes déterministes et limites de ressources ;
- invariants vérifiés entre AST, HIR, IR et backend avec origine source conservée ;
- conservation des spans pendant la croissance de l'AST et jusqu'aux diagnostics ;
- stabilisation des imports et des appels C générés ;
- correction de l'ambiguïté entre division et séparateurs de chemin ;
- prise en charge des espaces de noms imbriqués ;
- exclusion des symboles de `space` des collisions globales ;
- déclarations POSIX explicites pour les compilations C17 portables ;
- ajout du socle de backend C17, du nommage stable et de l'émission de traduction.

#### Interface CLI et configuration

- commandes documentées et testées : `compile`, `check`, `run`, `lex`,
  `parse`, `explain`, `help` et `version` ; `compile` reste la commande par
  défaut ;
- inspection du pipeline avec `--emit-tokens`, `--emit-ast`, `--emit-hir`,
  `--emit-ir` et `--emit-c` ;
- arrêt déterministe après le lexer, le parser ou l'analyse sémantique avec
  `--stop-after-lexer`, `--stop-after-parser` et `--stop-after-sema` ; les
  combinaisons incompatibles sont rejetées avant le lancement du pipeline ;
- contrôle explicite de la validation UTF-8 avec `--validate-utf8` et
  `--no-validate-utf8` ;
- prise en charge de `--quiet`, `--verbose`, `--color`, `--no-color`,
  `--timings`, `-o`, `--debug-info`, `--line-directives` et de l'alias
  `--debug` ;
- commande `explain` reliée au registre des diagnostics, avec rendu terminal
  ou JSON ;
- chargement de `vitte.toml`, de `.vitte/config` ou du chemin indiqué par
  `VITTE_CONFIG_FILE`, puis fusion des variables `VITTE_*` et des options CLI ;
- recherche de modules dans les chemins installés et ceux ajoutés par
  `VITTE_MODULE_PATH`, avec validation des doublons et des options conflictuelles ;
- sorties de compilation et exécution séparées : `run` transmet les arguments
  placés après `--` au programme généré.

### Grammaire et langage

- EBNF canonique synchronisée avec le parser, les tokens, la documentation et
  les éditeurs ;
- génération et contrôle automatiques de la couverture des mots-clés et opérateurs ;
- ajout de `print` dans la grammaire et le frontend ;
- chemins de modules normalisés avec `::` ;
- opérateurs logiques recommandés `and`, `or` et `not` ;
- `&&`, `||` et `!` conservés pour le diagnostic de compatibilité et orientés
  vers leur écriture canonique ;
- récupération parser après `;`, `,`, `)`, `]` et `}` ;
- récupération dans les listes et déclarations imbriquées avec réduction des
  erreurs en cascade ;
- déclarations historiques `class`, `union`, `bits` et `flags` identifiées comme
  compatibilité et orientées vers les formes actuelles ;
- mots réservés vérifiés contre leur utilisation réelle dans le frontend et les
  modules fournis ;
- tables Unicode UAX #31 générées avec version documentée et tests des
  identifiants Unicode invalides.

#### Compatibilité lexicale et récupération

- séparation explicite entre `/` utilisé par la division et `/` utilisé dans
  une référence de module ;
- caractères et échappements invalides signalés avec leur emplacement et une
  correction proposée uniquement lorsqu'elle est non ambiguë ;
- fermeture manquante ou inattendue reliée à l'ouverture correspondante quand
  celle-ci est connue ;
- points de reprise bornés aux séparateurs et débuts de déclaration afin que
  le parser continue sur le reste du fichier sans répéter le même diagnostic ;
- récupération dédiée pour les listes d'arguments, champs, éléments de type,
  paramètres et déclarations imbriquées ;
- maintien des spans lors de la croissance des nœuds afin que les messages
  restent attachés au texte réellement consommé.

### Système de types et analyse sémantique

- politique centrale de conversion exposant `exact`, `safe`, `lossy` et
  `incompatible` ;
- réutilisation de cette politique pour affectations, appels, champs, retours,
  tableaux, pointeurs et références ;
- refus des conversions implicites pouvant perdre une plage ou une précision ;
- conversions numériques contrôlées selon les bits représentables, y compris
  les conversions `char` vers flottant ;
- conversions explicites conservées avec `as` ;
- diagnostics de mismatch enrichis avec contexte, types attendu et trouvé,
  paramètre, champ ou type de retour concernés ;
- contrôles des symboles inconnus, types inconnus, visibilité privée, alias,
  mutabilité, exports, appels et arité ;
- suggestions de noms proches pour symboles, types, procédures et modules ;
- aides de conversion ajoutées lorsque le cast explicite est sûr ou nécessaire.

#### Contexte des erreurs de type

- appels : index, nom du paramètre, type attendu et type fourni sont conservés ;
- champs : nom du champ, type déclaré et type de la valeur sont affichés ;
- retours : procédure concernée, type déclaré et expression retournée sont
  reliés dans le même diagnostic ;
- affectations : mutabilité de la cible et type de la nouvelle valeur sont
  distingués ;
- conversions : la suggestion `as` n'est produite que si la conversion est
  explicite et vérifiable, tandis qu'une conversion implicite perdante reste
  une erreur.

### Imports et modules

- branchement du graphe `src/bootstrap/import` au chargeur CLI ;
- imports transitifs chargés une seule fois grâce au cache ;
- détection des cycles avec chaîne complète ;
- chemins canoniques et résolution compatible avec les liens symboliques ;
- détection des conflits d'alias ;
- vérification de la visibilité des symboles privés ;
- origine exacte des déclarations importées ;
- propagation des spans `use` avec fichier, ligne, colonne, module, déclaration
  et alias ;
- suggestions de chemins de modules proches en cas de module introuvable.

#### Contrat de résolution

- noms composés acceptés avec `.` ou `/` et normalisés vers un fichier
  `module/submodule.vit` ou `module/submodule/mod.vit` ;
- noms vides, séparateurs répétés, séparateurs finaux, segments `..` et noms
  dépassant la limite configurée refusés avant tout accès filesystem ;
- répertoires de l'importeur essayés avant les chemins globaux ;
- cache borné indexé par nom logique et base de l'importeur ; les buffers
  appartiennent au cache et les résultats ordinaires les empruntent ;
- identité physique utilisée pour éviter de charger deux fois un même fichier
  atteint par des chemins relatifs différents ou par un lien symbolique ;
- cycle diagnostiqué avec la chaîne complète `A -> B -> ... -> A` et limite de
  profondeur appliquée indépendamment ;
- alias enregistrés comme liaisons du module importeur : deux cibles ne peuvent
  pas partager le même alias ;
- origine de la déclaration `use` conservée jusque dans la sortie machine,
  y compris pour une déclaration privée ou un conflit d'alias.

### Diagnostics

- modèle commun pour les sorties texte, JSON, SARIF et LSP ;
- conservation des mêmes codes, sévérités, contextes, types, aides, causes,
  suggestions, origines et emplacements dans chaque format ;
- ajout de `--diagnostic=lsp` ;
- conversion correcte des positions LSP UTF-8, UTF-16 et UTF-32 ;
- conversion de secours des spans d'import et de driver sans gestionnaire de source ;
- aides ciblées pour ponctuation manquante, chemins `::`, opérateurs logiques,
  caractères invalides et échappements non valides ;
- rapprochement des symboles visibles pour les fautes de frappe ;
- récupération syntaxique limitant les diagnostics redondants ;
- chaînes d'origine et emplacements liés conservés pour les imports, paramètres,
  déclarations privées et retours ;
- tests automatiques de parité entre les quatre formats.

#### Contrat de diagnostic

- codes publics regroupés par domaine : infrastructure `E0000–E0099`, lexer
  `E0100–E0199`, parser `E0200–E0299`, imports `E0300–E0399`, noms
  `E0400–E0499`, types `E0500–E0599`, appels `E0600–E0699`, contrats
  `E0700–E0799`, génériques `E0800–E0899` et backend C17 `E0900–E0999` ;
- sévérités `note`, `help`, `warning`, `error` et `fatal` séparées de l'origine
  (`lexer`, `parser`, `import`, résolution de noms, typage, backend, etc.) ;
- causes ordonnées, notes, aides et suggestions portent chacun leur propre
  structure ; une suggestion indique sa plage, son remplacement et son degré
  d'applicabilité ;
- le registre associe à chaque code son titre, son explication, sa phase et sa
  politique de remédiation ; `vitte explain E0501` réutilise ces mêmes données ;
- le sac de diagnostics est borné, conserve un ordre stable, compte les
  diagnostics supprimés et transforme les avertissements en erreurs lorsque
  `warnings_as_errors` est actif ;
- `VITTE_BACKTRACE=1` ajoute une trace native best-effort aux erreurs internes
  sur macOS et Linux ;
- les rendus JSON, SARIF et LSP regroupent les diagnostics d'une même phase en
  un document valide, au lieu de concaténer plusieurs fragments ;
- les positions LSP sont converties en UTF-8, UTF-16 ou UTF-32 selon le client,
  avec un chemin de secours pour les spans provenant du driver ou des imports.

### Lexer, parser et sûreté mémoire

- tests de contrat des tokens et des diagnostics lexer ;
- fuzzing déterministe lexer/parser sur des milliers d'entrées binaires ;
- fuzzing du graphe d'import avec modules aléatoires, doublons et cycles ;
- tests ASan et UBSan intégrés à `make -C src sanitize-test` ;
- tests de limites, d'échecs d'allocation et de nettoyage des sous-systèmes ;
- huit compilations indépendantes exécutées en parallèle pour vérifier la
  réentrance du driver ;
- chemins d'échec d'allocation et nettoyage après erreur couverts par les tests.

#### Cibles de vérification

- `make -C src build` compile le bootstrap C17 ;
- `make -C src verify` vérifie les sources, en-têtes, contrats et empreintes ;
- `make -C src test-frontend` regroupe lexer, parser, diagnostics, Unicode,
  imports, objets et régressions bootstrap ;
- `make -C src test-diagnostics` vérifie les formats, l'aide par code et les
  positions liées ;
- `make -C src test-import-fuzz`, `test-type-conversions` et
  `test-concurrent` couvrent respectivement les graphes aléatoires, les
  conversions et les compilations parallèles ;
- `make -C src sanitize-test` exécute les mêmes chemins sous ASan et UBSan ;
- le corpus de régression couvre les erreurs lexer, les listes imbriquées, les
  cycles d'import, les alias, la visibilité privée, les appels et les retours.

### Module JSON

- correction de l'analyse des nombres entiers, flottants et limites numériques ;
- helpers publics de validation et helper de parsing pour les tests round-trip ;
- alignement des tests du parser sur les sémantiques du module core ;
- couverture étendue de l'encodeur, des nombres, des chaînes et des erreurs ;
- documentation complète du module JSON et de ses sous-composants.

### Runtime et backend

- contrat du runtime documenté ;
- manifeste de paquet runtime ajouté ;
- égalité des chaînes prise en charge dans le backend C17 ;
- appels C générés et imports runtime stabilisés ;
- exclusion des répertoires de build lors de la copie des arbres sources ;
- artefacts de build ajoutés aux règles d'exclusion.

#### Sorties et stabilité du backend

- génération C avec noms stables et conservation de l'origine source pour les
  erreurs de codegen ;
- égalité de chaînes disponible dans le runtime C17 ;
- manifeste runtime utilisé lors de la validation des modules inclus dans les
  paquets ;
- les étapes HIR, IR et C17 exposent leurs invariants même lorsque la baisse
  complète reste désactivée ;
- les sorties incomplètes sont signalées comme telles par `compile` et `run`,
  sans annoncer à tort un exécutable terminé.

### Éditeurs et outils

- extension VS Code complète avec diagnostics, LSP, complétion, navigation,
  formatage, hover, tokens sémantiques, débogage et vues de projet ;
- configurations et fixtures mises à jour pour VS Code, Geany, Tree-sitter,
  Vim, Emacs et Nano ;
- documentation des capacités LSP et des diagnostics ;
- exemples et commandes CLI actualisés pour le comportement 0.1.1.

### CI, portabilité et installateurs

- workflows CI compilateur et extension séparés puis réunis dans les gates
  complètes ;
- tests headless rendus déterministes et diagnostics Linux enrichis ;
- actions Node 24 et runners Ubuntu 24.04 ;
- builds CMake et vérification C/C++ ajoutés ;
- vérifications GCC, Clang et cibles Windows ajoutées aux scripts de portabilité ;
- sorties ASan et ARM64 portables ;
- installateurs macOS universels et cible macOS i386 historique ;
- installateurs Raspberry Pi 1 avec contrôles pilotés par environnement ;
- scripts Windows, Solaris, Linux, FreeBSD et BSD mis à jour ;
- validation automatique des modules embarqués dans les paquets ;
- manifeste runtime, licences et sommes de contrôle actualisés.

#### Artefacts publiés

- paquets macOS arm64, x86_64, universal et universal2 en versions `.dmg` et
  `.pkg` ;
- configuration macOS i386 historique conservée dans une archive dédiée ;
- installateurs Linux, Windows, FreeBSD et autres BSD vérifiant la présence et
  le contenu des modules embarqués ;
- sommes SHA-256 produites avec chaque artefact ;
- sorties ARM64 et contrôle Raspberry Pi 1 intégrés aux vérifications de
  publication ;
- métadonnées, licences et numéro de version alignés sur `0.1.1`.

### Documentation

- nouvelle documentation d'architecture du compilateur et du bootstrap ;
- guides de migration native, mapping des fichiers et utilisation du driver ;
- références de grammaire, modules, packages, diagnostics et API ;
- livre et pages web régénérés, index de recherche et données de statut mis à jour ;
- rapports de sûreté mémoire, compatibilité, maturité des paquets et support ;
- modèles de notes de version et procédures de publication ajoutés.

#### Références techniques ajoutées

- contrat du gestionnaire de sources : identifiants de fichiers, plages d'octets,
  lignes, colonnes et conversions de position ;
- contrat du graphe d'import : propriété du cache, identité physique/logique,
  arêtes, alias et cycles ;
- contrat des diagnostics : codes, phases, sévérités, aides, causes et formats ;
- documentation du driver et du mapping entre l'ancienne arborescence et
  `src/bootstrap` ;
- documentation des options de configuration, des variables d'environnement,
  des modules JSON et des capacités LSP ;
- contrôle de synchronisation README/EBNF/code ajouté aux vérifications de
  documentation.

### Migration depuis 0.1.0

- utiliser `and`, `or`, `not` dans les nouvelles expressions logiques ;
- utiliser `::` dans les chemins d'import ;
- utiliser `as` pour toute conversion numérique potentiellement perdante ;
- utiliser `--diagnostic=lsp` pour obtenir des diagnostics LSP depuis la CLI ;
- vérifier les alias et la visibilité des symboles privés dans les imports ;
- mettre à jour les scripts qui supposaient l'ancienne arborescence `src/src`.

Les scripts et intégrations qui consommaient uniquement la sortie terminal
doivent accepter le champ `phase` et les aides structurées des sorties JSON,
SARIF ou LSP. Les consommateurs d'import doivent utiliser l'origine attachée
à l'arête `use`, plutôt que reconstruire un chemin depuis le message texte.

### Limites connues

- la cible native expérimentale atteint maintenant la génération C17 en
  réutilisant les étapes bootstrap C pour parser, analyse sémantique, HIR, IR et
  codegen ; ces modules ne sont pas encore portés en Vitte ;
- la sortie C17 du pipeline API est temporaire et supprimée ; l'écriture vers
  un fichier, la compilation native et l'édition de liens ne sont pas raccordées
  à cette API ;
- `compile` et `run` signalent explicitement leur étape manquante au lieu de
  déclarer une compilation ou une exécution réussie ;
- certaines déclarations de compatibilité sont reconnues pour la récupération,
  mais ne constituent pas encore des formes sémantiques complètes.
