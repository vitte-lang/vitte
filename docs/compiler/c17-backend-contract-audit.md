# Audit du contrat du backend C17

Cet audit compare les headers bootstrap `src/bootstrap/backend/c17/*.h` aux
modules natifs `src/compiler/backend/c17/*.vitte`. Le contrôle des noms est
statique : pour AST/IR, il normalise le préfixe `vitte_c17_` en `c17_`. Il ne
prouve pas une compatibilité binaire ni une équivalence complète des sorties.

Les deux arbres restent deux implémentations séparées : le bootstrap C/H garde
son ABI C et son stockage manuel ; les modules Vitte utilisent les types et
conteneurs du langage. Les structures Vitte ne sont donc pas des remplacements
binaires des structures déclarées dans les headers C.

## Surface d’API

Les comptes C incluent les helpers inline déclarés dans les headers. « Noms
communs » indique les fonctions du header retrouvées dans le module Vitte,
après normalisation des préfixes pour les deux émetteurs.

| Module | Fonctions du header C/H | Noms communs | Écart principal |
| --- | ---: | ---: | --- |
| `backend` | 69 | 69 | Les deux helpers FILE ont maintenant des wrappers Vitte; le type callback demeure une représentation native distincte de l’ABI C. |
| `ast_emitter` | 7 | 2 | Les cinq fonctions C de compilation, édition de liens et exécution ne sont pas dans l’émetteur Vitte. |
| `ir_emitter` | 6 | 2 | Les quatre fonctions C de compilation, production d’objet et exécution ne sont pas dans l’émetteur Vitte. |
| `module` | 51 | 51 | Noms de fonctions concordants; stockage différent. |
| `naming` | 42 | 42 | Noms de fonctions concordants; stockage différent. |
| `options` | 42 | 42 | Noms de fonctions concordants. |
| `program` | 56 | 56 | Noms de fonctions concordants; stockage différent. |
| `translation_unit` | 49 | 49 | Noms de fonctions concordants; stockage différent. |
| `writer` | 58 | 58 | Noms concordants, avec deux opérations Vitte supplémentaires pour la sortie par chemin. |

Les variantes et l’ordre des valeurs des enums d’erreur, d’état, de cible, de
section, d’option, de module, de programme, d’unité de traduction et de writer
concordent. Les types de configuration et les statistiques gardent les mêmes
champs logiques dans les modules comparables. Cela ne rend pas les signatures
interchangeables au niveau C : les headers utilisent notamment `FILE *`,
`const char *`, pointeurs et longueurs explicites ; Vitte utilise des chaînes,
des références optionnelles et des tableaux dynamiques.

### `module.h` / `module.vitte` — inventaire de l’API publique

Les 51 fonctions déclarées dans `module.h`, helpers inline compris, ont une
fonction de même nom dans `module.vitte` :

| Famille | Fonctions |
| --- | --- |
| Noms d’enums et configuration | `vitte_c17_module_error_name`, `vitte_c17_module_state_name`, `vitte_c17_include_kind_name`, `vitte_c17_symbol_kind_name`, `vitte_c17_runtime_requirement_name`, `vitte_c17_module_config_default` |
| Cycle de vie et validation | `vitte_c17_module_init`, `vitte_c17_module_destroy`, `vitte_c17_module_is_valid`, `vitte_c17_module_validate` |
| Source | `vitte_c17_module_set_source` |
| Dépendances | `vitte_c17_module_add_dependency`, `vitte_c17_module_find_dependency` |
| Inclusions | `vitte_c17_module_add_include` |
| Symboles | `vitte_c17_module_add_symbol`, `vitte_c17_module_find_symbol` |
| Features | `vitte_c17_module_require_feature`, `vitte_c17_module_has_feature` |
| Runtime | `vitte_c17_module_require_runtime`, `vitte_c17_module_requires_runtime`, `vitte_c17_module_runtime_mask`, `vitte_c17_runtime_requirement_fits_mask` |
| Fragments et scellement | `vitte_c17_module_add_fragment`, `vitte_c17_module_section_fragment_count`, `vitte_c17_module_seal` |
| Émission | `vitte_c17_module_emit`, `vitte_c17_module_emit_adapter`, `vitte_c17_module_generate` |
| Identité, état et statistiques | `vitte_c17_module_name`, `vitte_c17_module_name_length`, `vitte_c17_module_source_path`, `vitte_c17_module_source_file_id`, `vitte_c17_module_state`, `vitte_c17_module_last_error`, `vitte_c17_module_stats` |
| Comptes de collections | `vitte_c17_module_dependency_count`, `vitte_c17_module_include_count`, `vitte_c17_module_symbol_count`, `vitte_c17_module_feature_count`, `vitte_c17_module_fragment_count` |
| Accès indexé | `vitte_c17_module_dependency_at`, `vitte_c17_module_include_at`, `vitte_c17_module_symbol_at`, `vitte_c17_module_feature_at`, `vitte_c17_module_fragment_at` |
| Prédicats d’état | `vitte_c17_module_is_building`, `vitte_c17_module_is_sealed`, `vitte_c17_module_is_emitting`, `vitte_c17_module_is_emitted`, `vitte_c17_module_has_failed`, `vitte_c17_module_has_error` |

Les rôles et types logiques des paramètres concordent. Les signatures ne sont
toutefois pas des déclarations C interchangeables : les chaînes C sont
représentées par un pointeur et une longueur, contre un `string` Vitte ; les
pointeurs d’objet deviennent des références, et les résultats pointeur/null
deviennent des valeurs optionnelles (`?string` ou `?&T`). Les entiers C
`uint32_t`, `uint64_t` et `size_t` correspondent respectivement aux types
Vitte `u32`, `u64` et `usize`. L’adaptateur conserve des pointeurs opaques dans
les deux API, avec des types de pointeur différents. La structure mémoire de
`C17Module` reste propre à chaque implémentation.

Le header C fournit en plus quatre macros de commodité —
`VITTE_C17_MODULE_ADD_SYSTEM_INCLUDE`,
`VITTE_C17_MODULE_ADD_LOCAL_INCLUDE`,
`VITTE_C17_MODULE_REQUIRE_FEATURE` et `VITTE_C17_MODULE_ADD_FRAGMENT` — qui
appellent les fonctions listées ci-dessus en calculant la longueur des
littéraux. Elles n’ont pas d’équivalent sous forme de macro/export Vitte ;
leurs opérations restent accessibles par appel direct aux procédures.

### `module.h` / `module.vitte` — formes et stockage

| Donnée logique | Forme C/H | Forme Vitte | Parité de représentation |
| --- | --- | --- | --- |
| État, erreur, type d’inclusion, type de symbole, exigence runtime | `enum` C | `pick` Vitte | Variantes et ordre concordants ; aucune garantie de layout ABI commun. |
| Configuration | Six champs `bool` | Six champs `bool` | Même champs et même ordre logique. |
| Dépendance | `char *name`, `size_t name_length`, `bool public_dependency` | `string name`, `bool public_dependency` | Chaîne possédée dans les deux formes ; longueur explicite et NUL final côté C. |
| Inclusion | enum, `char *path`, `size_t path_length` | enum, `string path` | Même information ; longueur explicite côté C. |
| Symbole | enum, `char *name`, longueur, `bool exported`, `uint64_t source_id` | enum, `string name`, `bool exported`, `u64 source_id` | Champs logiques concordants ; longueur explicite côté C. |
| Feature | `char *name`, `size_t name_length` | `string name` | Même information ; longueur explicite côté C. |
| Fragment | section, `char *text`, longueur, `uint64_t source_id`, `uint64_t sequence` | section, `string text`, `u64 source_id`, `u64 sequence` | Même information ; le C garde un buffer NUL-terminé et une longueur faisant autorité. |
| Statistiques | Huit champs `size_t` | Huit champs `usize` | Noms, ordre logique et compteurs concordants. |
| Module | Pointeurs/longueurs pour nom et chemin ; pointeur, compte et capacité pour chaque tableau | `string`, `?string` et tableaux dynamiques | Vitte ne publie ni pointeurs, ni longueurs de chaînes séparées, ni capacités de tableaux. |

Les correspondances d’entiers sont `uint32_t` ↔ `u32`, `uint64_t` ↔ `u64` et
`size_t` ↔ `usize`. Les deux formes gardent aussi `magic`, l’état, la dernière
erreur, la configuration, l’identifiant source, le masque runtime, le total
d’octets des fragments, la prochaine séquence et les statistiques. Les valeurs
par défaut des deux marqueurs magic sont identiques ; le C peut les remplacer
par macros de compilation, tandis que Vitte les fixe comme constantes.

Dans le C, le module possède les buffers de chaînes et les tableaux alloués sur
le tas ; leurs champs `*_length`, `*_count` et `*_capacity` sont stockés
explicitement. Les tableaux commencent sans allocation et le code C les
agrandit au besoin. Dans Vitte, le module conserve les valeurs `string` et les
tableaux natifs du langage ; leur longueur logique est disponible par les
opérations du langage, sans champ de capacité dans `C17Module`. Ce modèle garde
le même contenu logique sans partager la disposition mémoire C.

Le header C définit une capacité initiale générale de 8 et une capacité initiale
de fragments de 16. Dans `module.c`, la fonction de croissance consultée part
de la capacité générale de 8 pour ses tableaux ; la macro de capacité initiale
des fragments est déclarée, mais aucune référence à cette macro n’a été trouvée
dans le fichier. Ce détail est à traiter avec l’alignement des limites et
constantes, sans l’imposer au layout Vitte.

### `module.h` / `module.vitte` — limites et largeurs

- Les limites par défaut du nom (4 096 octets) et du chemin source (1 048 576
  octets) sont identiques. `module.vitte` fixe ces valeurs dans ses constantes ;
  les macros C correspondantes peuvent être remplacées à la compilation.
- Le C borne la taille d’un fragment par défaut à `SIZE_MAX`. Vitte déclare
  maintenant `MODULE_MAX_FRAGMENT_BYTES = usize.MAX` et applique ce plafond dans
  `vitte_c17_module_add_fragment`. Les deux implémentations cumulent les octets
  des fragments par addition saturante, avec `SIZE_MAX`/`usize.MAX` comme valeur
  de saturation.
- Les tailles et compteurs de collections sont représentés par `size_t` en C
  et `usize` en Vitte. Les identifiants source restent en `uint32_t`/`u32` ;
  source ID, séquence et masque runtime sont en `uint64_t`/`u64`.
- Le masque runtime est de 64 bits des deux côtés. Les valeurs actuelles sont
  `None = 0`, exigences 1 à 10, puis `Count = 11`. Les deux API refusent les
  exigences hors de l’enum et celles dont l’indice ne tient pas dans 64 bits ;
  le header C ajoute une assertion que `RUNTIME_COUNT <= 64`.
- Les limites C peuvent être personnalisées par macros de compilation, sans
  mécanisme équivalent dans les constantes Vitte actuelles. La parité des
  limites porte donc sur les valeurs par défaut documentées ci-dessus.

### `module.h` / `module.vitte` — configuration par défaut

Les six champs de `C17ModuleConfig` portent les mêmes noms et ont les mêmes
valeurs par défaut dans les deux implémentations :

| Champ | C | Vitte | Effet couvert |
| --- | --- | --- | --- |
| `deterministic` | `true` | `true` | Trier les collections au scellement. |
| `deduplicate_dependencies` | `true` | `true` | Réutiliser une dépendance identique. |
| `deduplicate_includes` | `true` | `true` | Réutiliser une inclusion identique. |
| `reject_duplicate_symbols` | `true` | `true` | Refuser un nom de symbole en conflit. |
| `reject_duplicate_fragments` | `false` | `false` | Autoriser les fragments identiques. |
| `emit_empty_sections` | `false` | `false` | Omettre les sections sans contenu. |

`module_init` applique aussi la même règle : une configuration nulle/absente
utilise ces défauts ; une configuration fournie est copiée telle quelle. Aucun
champ ne nécessite de normalisation ou de validation supplémentaire.

### `module.h` / `module.vitte` — initialisation

- Les deux rejettent un nom vide ou dépassant la limite avant de modifier le
  module ; les appels invalides ne posent pas `last_error`. C rejette aussi un
  pointeur module ou nom nul, cas que les références et chaînes Vitte ne
  représentent pas directement.
- En cas de succès, les deux fixent l’état à `Building`, l’erreur à `None`,
  l’identifiant source à zéro, le chemin à absent, les collections et le masque
  runtime à vide, la séquence à zéro et les statistiques à zéro. Elles copient
  la configuration fournie ou appliquent ses défauts.
- C copie d’abord le nom ; si cette allocation échoue, le module reste intact.
  La copie de chaîne Vitte n’expose pas un résultat d’allocation récupérable
  équivalent.
- Réinitialiser un module encore actif est interdit : après validation du nom,
  Vitte le refuse avec `InvalidState` et conserve ses données ; le header C exige
  un `destroy` avant réutilisation, car `init` écraserait sinon la structure
  sans libérer son nom, chemin ni tableaux. Cette règle reste une précondition
  côté C, qui ne détecte pas l’appel fautif.

### `module.h` / `module.vitte` — identité source et accesseurs

`set_source` exige `Building`, copie le chemin et remplace l’identité source
précédente seulement après la copie C réussie. Les deux versions autorisent un
chemin vide : le C exige un pointeur non nul et une longueur zéro, tandis que
Vitte reçoit `""`. Dans les deux cas, le chemin est présent mais vide, ce qui
reste distinct d’un chemin absent ; l’identifiant fourni est conservé. Les
limites maximales du chemin concordent. L’échec d’allocation C garde l’ancien
chemin et l’ancien identifiant ; l’allocation Vitte n’a pas de résultat
récupérable exposé.

Les accesseurs d’identité/statistiques appellent `is_valid` et ont les mêmes
valeurs de repli si le magic ou l’état rend le module invalide : nom et chemin
absents, longueur et identifiant à zéro, statistiques nulles. `module_state` et
`module_last_error` vérifient seulement le magic ; après `destroy`, l’état
interne vaut `Destroyed`, mais `module_state` retourne `Uninitialized` et
`module_last_error` retourne `InvalidModule`. Si le magic reste vivant avec un
état invalide, ces deux accesseurs renvoient les champs stockés. Les comptes,
accès indexés, prédicats d’état, `has_error` et `runtime_mask` lisent directement
leurs champs ; après destruction les collections et le masque sont vides, mais
`has_error` peut rester vrai, car `destroy` ne remet pas `last_error` à zéro.

Le helper `runtime_mask` avait un écart : Vitte renvoyait zéro sur un module
invalide, alors que l’inline C retourne directement le masque de tout pointeur
non nul. Vitte renvoie maintenant le champ directement. Les pointeurs C nuls
renvoient zéro ; une référence Vitte ne peut pas représenter ce cas.

### `module.h` / `module.vitte` — transitions d’état

| État | Entrée/sortie | Opérations permises |
| --- | --- | --- |
| `Uninitialized` | État avant `init`. | Initialiser. L’objet n’est pas valide. |
| `Building` | État après `init`. | Ajouter source, dépendances, inclusions, symboles, features, runtime et fragments ; valider, sceller ou détruire. |
| `Sealed` | `seal`, ou scellement automatique au début de `emit`. | Lecture ou émission avec un backend en génération ; répéter `seal` réussit ; détruire. Toute mutation est refusée. |
| `Emitting` | État transitoire pendant `emit`. | Émission en cours. |
| `Emitted` | Émission complète. | Lecture ou destruction. Un nouvel `emit` échoue avec `InvalidState` sans changer l’état. |
| `Failed` | Échec d’écriture pendant `emit`. | Lecture ou destruction. L’émission et les mutations ne reprennent pas. |
| `Destroyed` | `destroy` sur un module initialisé. | Réinitialiser le stockage. `is_valid` retourne faux ; l’accesseur `module_state` retourne `Uninitialized` car le magic n’est plus vivant. |

Les transitions principales sont `Uninitialized -> Building -> Sealed ->
Emitting -> Emitted`, avec `Emitting -> Failed` si une écriture échoue. Tout
état initialisé peut être détruit ; `Destroyed -> Building` est possible en
réinitialisant le stockage.

Les transitions de scellement et d’émission concordent. Les mutations de
contenu (`set_source` et les ajouts) exigent `Building`; `seal`, `emit`,
`destroy` et `init` suivent les règles de cycle de vie ci-dessus. Un refus pour
mauvais état d’une mutation de contenu ne change pas l’état. Vitte refuse aussi
`init` sur un magic encore vivant ; côté C, l’interdiction de réinitialiser un
module actif est une précondition documentée, car `init` ne peut pas distinguer
sans risque un stockage frais d’un objet déjà initialisé.
`module_fail` enregistre une erreur sans passer à `Failed`. Un module en état
`Failed` reste valide pour lecture et destruction, mais ne peut pas être scellé
ni réémis. `emit` exige un backend en génération ; il scelle d’abord un module
`Building`, entre ensuite dans `Emitting`, puis fixe `Emitted` ou `Failed`.
Comme `emit` scelle avant de vérifier l’état du backend, un backend valide mais
pas en `Generating` laisse un module initialement `Building` en `Sealed` et pose
`InvalidState`. Les autres refus d’état de `emit` et ceux de `seal` ne changent
pas l’état.
`validate` est une lecture et ne modifie ni l’état ni l’erreur.

Le module n’a pas d’API `clear_error`. Une initialisation, un scellement réussi
depuis `Building` et une émission réussie effacent `last_error`. Les mutations
réussies ordinaires et un `seal` répété sur un module déjà `Sealed` laissent
l’erreur précédente en place. Les échecs d’arguments et les gardes d’état des
mutations renvoient généralement `false` sans modifier `last_error`. Les
échecs de doublon, conflit et allocation enregistrent leur erreur sans
transition vers `Failed`; `seal` et `emit` enregistrent aussi `InvalidState`
sur leurs transitions interdites. Seul l’échec de sortie pendant l’émission
fait passer le module à `Failed`.

### `module.h` / `module.vitte` — validation et erreurs

Les deux API exposent les mêmes catégories et noms d’erreur :
`none`, `invalid-module`, `invalid-argument`, `invalid-state`, `overflow`,
`out-of-memory`, `duplicate`, `conflict`, `invalid-section`, `output`,
`corruption` et `count`. `module_fail` n’écrit dans `last_error` que si le magic
du module est vivant ; il ne change jamais l’état.

`module_validate` est une vérification profonde sans effet de bord. C et Vitte
contrôlent les bornes du nom et du chemin, les dépendances non vides et uniques,
les inclusions de kind valide et uniques par `(kind, path)`, les symboles non
vides de kind valide, les features non vides et uniques, les sections des
fragments, le dépassement arithmétique de leur taille totale, la cohérence de
`fragment_bytes` et la borne `emitted_fragments <= fragment_count` en état
`Emitted`. Comme dans C, la validation des symboles ne recherche pas les noms
dupliqués ; leur détection appartient à `add_symbol` selon la configuration.

Le C vérifie en plus les invariants de ses tableaux manuels : pointeur présent
si le compte est non nul et compte inférieur ou égal à la capacité. Vitte n’a
pas de champs pointeur/capacité séparés ; ses tableaux natifs fournissent cette
garantie de représentation. Les deux validateurs renvoient simplement `false`
sur corruption ou incohérence, sans remplir `last_error`. `seal` ne lance pas
`validate` implicitement dans l’une ou l’autre version.

### `module.h` / `module.vitte` — ajouts, doublons et scellement

Toutes les opérations d’ajout exigent l’état `Building`. Les dépendances,
inclusions, symboles, features et fragments possèdent leur texte dans les deux
implémentations. Les entrées invalides ou les mauvais états renvoient `false`
sans modifier `last_error`; les doublons signalés, dépassements de compte et
échecs d’allocation passent par `module_fail`. Les deux distinguent un compte à
`SIZE_MAX`/`usize.MAX` (`Overflow`) d’un échec de réservation ou d’allocation
(`OutOfMemory`).

- Dépendances : un nom déjà présent est idempotent si
  `deduplicate_dependencies` vaut `true`; une dépendance publique peut alors
  promouvoir l’entrée existante en publique. Si la déduplication est désactivée,
  l’appel renvoie `false` avec `Duplicate`, mais cette promotion publique a
  quand même lieu avant le retour.
- Inclusions : l’identité est le couple `(kind, path)` et le chemin vide est
  refusé. `deduplicate_includes` décide si une répétition réussit sans ajouter
  d’entrée ou renvoie `Duplicate`.
- Symboles : une répétition de même nom, kind et `source_id` est idempotente et
  peut promouvoir `exported` à `true`. Un même nom avec une autre identité
  renvoie `Conflict` si `reject_duplicate_symbols` est activé ; sinon une
  nouvelle entrée est ajoutée. `find_symbol` renvoie la première entrée portant
  ce nom.
- Features : les noms non vides sont ajoutés une fois ; répéter une feature
  réussit sans nouvelle entrée. Les exigences runtime valides sont aussi
  idempotentes et se représentent par un bit du masque.
- Fragments : le doublon est défini par `(section, texte)`, sans tenir compte de
  `source_id`. Il est refusé avec `Duplicate` seulement si
  `reject_duplicate_fragments` est activé. Le texte vide est permis. Chaque
  ajout reçoit une séquence `u64` qui s’incrémente modulo 2^64 et sert de dernier
  critère de tri.

Avec `deterministic == true`, `seal` trie les dépendances par nom, les
inclusions par `(kind, path)`, les symboles par `(nom, kind, source_id)`, les
features par nom et les fragments par `(section, source_id, sequence)`. Les
comparaisons de chaînes portent sur les octets dans les deux versions. Avec le
mode déterministe désactivé, les entrées gardent leur ordre d’ajout. Le scellement
répété d’un module déjà `Sealed` réussit sans retrier ni effacer une erreur
précédente ; le premier scellement depuis `Building` trie selon la configuration,
passe à `Sealed` et remet `last_error` à `None`. `section_fragment_count` compte
les fragments de la section valide demandée et renvoie zéro pour un module ou
une section invalide.

### `module.h` / `module.vitte` — émission et adaptateur

`emit` exige un module et un backend valides. Il scelle automatiquement un
module `Building`, puis exige `Sealed` et un backend déjà en `Generating` ; cette
fonction n’appelle ni `backend_begin` ni `backend_finish`. Un module hors état
requis ou un backend valide mais pas en génération pose `InvalidState` ; des
objets invalides renvoient `false` sans cette mutation. L’émission écrit le
commentaire d’identité si les commentaires sont activés, les macros de features,
les inclusions, puis les fragments dans le même ordre fixe de dix sections,
des déclarations anticipées à l’épilogue. Les dépendances et symboles restent
des métadonnées du module et ne sont pas émises par cette fonction.

Les deux versions remettent les statistiques à zéro au début de l’émission. Les
compteurs de fragments émis progressent après chaque fragment écrit avec succès
et comptent aussi un fragment vide, dont le nombre d’octets émis est zéro. En
cas d’échec d’écriture, les statistiques d’émission partielles restent visibles,
le module passe à `Failed` et `last_error` devient `Output`. En cas de succès,
les comptes des collections et octets stockés sont finalisés, l’état devient
`Emitted` et l’erreur revient à `None`.

`emit_adapter` interprète le pointeur opaque d’unité de traduction comme un
`C17Module`, refuse le pointeur nul et ignore `user_data`. `generate` valide les
deux objets puis appelle `backend_generate` avec cet adaptateur et le module
comme unité opaque ; `backend_generate` encadre l’appel par `backend_begin`,
l’adaptateur puis `backend_finish` en cas de succès. L’émission directe et
`generate` partagent ainsi le même émetteur ; seule la seconde encadre l’appel
par le cycle de vie du backend.

### `module.h` / `module.vitte` — comptes, accès indexés et prédicats

Les cinq comptes de collections lisent directement les tailles stockées : le C
renvoie zéro pour un pointeur module nul, tandis que Vitte reçoit une référence.
Ils ne valident ni le magic ni l’état. Les cinq accesseurs indexés renvoient
l’entrée si l’indice est inférieur au compte et une valeur nulle/optionnelle
sinon ; eux aussi ne valident ni magic ni état. C vérifie en plus le pointeur
module nul, cas non représentable par la référence Vitte.

Après `destroy`, les tableaux et comptes sont vidés des deux côtés, donc les
comptes renvoient zéro et les accès indexés ne trouvent aucune entrée. Les
prédicats `is_building`, `is_sealed`, `is_emitting`, `is_emitted` et `has_failed`
comparent uniquement l’état stocké, sans vérifier le magic ; ils sont tous faux
sur l’état `Destroyed`. `has_error` teste uniquement `last_error != None`. Comme
`destroy` conserve cette erreur, `has_error` peut rester vrai après destruction,
comme indiqué pour les helpers directs plus haut.

### `module.h` / `module.vitte` — recherches et requêtes

`find_dependency`, `find_symbol` et `has_feature` vérifient la validité du module,
mais pas son état : ils restent consultables en `Building`, `Sealed`, `Emitted`
ou `Failed`, et renvoient absence après destruction. Le C exige en plus un
pointeur de nom non nul ; Vitte reçoit une chaîne. Les correspondances sont
exactes, sensibles à la casse et sans normalisation ; C compare la longueur puis
les octets. Une recherche vide ne trouve aucune entrée, car les noms ajoutés
sont non vides. Ces lectures ne modifient pas `last_error`.

Les fonctions `find_*` retournent l’entrée dans le tableau, pas une copie. En C,
un ajout qui réalloue le tableau ou le tri de `seal` peut invalider le pointeur
ou changer l’entrée située à son indice. Les références Vitte désignent aussi
les éléments du tableau natif et ne doivent pas être conservées à travers une
mutation qui ajoute ou trie des éléments.

`requires_runtime` est aussi une lecture sans restriction d’état : elle vérifie
la validité du module, que l’exigence se trouve dans l’enum et que son indice
tient dans le masque 64 bits, puis teste le bit correspondant. Une exigence
valide mais non présente renvoie `false`. Elle ne modifie pas l’erreur.

### `module.h` / `module.vitte` — noms d’enum et configuration par défaut

Pour chaque variante déclarée, les helpers `*_name` renvoient les mêmes chaînes
dans les deux implémentations, y compris les variantes sentinelles `Count` et
les valeurs `Invalid`/`None` lorsqu’elles existent. Les fonctions C utilisent
des `switch` sans branche `default` ; elles ne définissent donc pas de chaîne de
repli pour un entier hors des valeurs de l’enum. La parité documentée porte sur
les valeurs déclarées, pas sur des enums invalides fabriqués par conversion.

`module_config_default` reste identique : déterminisme et déduplication des
dépendances/inclusions activés, rejet des symboles dupliqués activé, rejet des
fragments dupliqués et émission des sections vides désactivés. C construit la
valeur après zéro-initialisation ; Vitte retourne directement la structure avec
ces six valeurs.

### `module.h` / `module.vitte` — destruction et réinitialisation

`destroy` est idempotent : si le magic n’est pas vivant, les deux versions ne
font rien. Avec un module initialisé, le C libère les chaînes possédées, chaque
chaîne d’entrée et les cinq tableaux, puis remet pointeurs, comptes, capacités
et longueurs d’identité à zéro/null. Vitte vide ses tableaux natifs et remet le
nom, le chemin et le masque runtime à leurs valeurs vides. Les deux fixent l’état
à `Destroyed` puis invalident le magic. La stratégie de libération et la capacité
des tableaux Vitte restent internes ; la structure publique n’expose pas ces
champs.

La destruction ne remet pas à zéro `last_error`, la configuration, les
statistiques, `source_file_id`, `fragment_bytes` ni `next_fragment_sequence`.
Ces champs bruts sont conservés dans les deux implémentations, mais les
accesseurs qui valident le magic renvoient leurs valeurs de repli après
destruction. `module_state` renvoie alors `Uninitialized`, `module_last_error`
renvoie `InvalidModule`, les comptes sont nuls et `has_error` peut toujours
refléter l’erreur conservée. Une réinitialisation après `destroy` est permise ;
la réinitialisation d’un module encore actif reste interdite, comme décrit dans
la section sur `init`.

### `program.h` / `program.vitte` — registre et propriété des modules

Le programme possède le stockage du registre, les noms copiés des modules, les
dépendances, les symboles et les caches d’ordre et de reachability. Il ne
possède pas les objets `C17Module` : le pointeur enregistré est emprunté et le
module doit rester vivant tant que le programme peut le consulter. `add_module`
refuse un nom vide, un nom hors limite, un dépassement de la limite de modules,
un pointeur de module déjà enregistré ou un nom déjà utilisé. Sur succès, il
attribue un ID non nul croissant, conserve l’indice d’enregistrement, invalide
l’ordre et la reachability calculés, puis efface `last_error`.

L’enregistrement ne valide pas le magic du module référencé. La vérification
des modules dépend de `validate_modules` lors de la validation complète du
programme. C exige des pointeurs non nuls pour le programme, le module et le
nom ; Vitte reçoit des références/chaînes et vérifie aussi que `name_length` ne
dépasse pas la chaîne avant d’en copier le préfixe demandé.

`reset` efface l’entrée, les dépendances, les symboles, l’entrée du programme
et les caches, remet les prochains IDs à 1, efface l’erreur et incrémente
`generation` jusqu’à sa saturation. Il conserve la configuration et les
capacités principales pour réutilisation. `destroy` libère le stockage du
registre et invalide le magic ; les objets modules empruntés restent au caller.
Les deux implémentations valident la configuration avant `init`, mais ne
détectent pas une réinitialisation sur un programme déjà actif. C écrase alors
les pointeurs possédés avec `memset` sans les libérer ; il faut détruire le
programme avant de le réinitialiser.

## Écarts relevés

### `backend.h` / `backend.vitte`

- Les enums, les champs de `BackendConfig`, `Target`, `SourceLocation` et
  `BackendStats`, ainsi que leurs valeurs par défaut et validations, concordent
  dans l’implémentation inspectée.
- Le champ callback C `diagnostic` s’appelle `diagnostic_handler` dans la forme
  Vitte. La forme Vitte conserve les 19 champs du backend, mais ce nom et les
  représentations de chaîne empêchent de traiter les deux formes comme un ABI
  partagé.
- `vitte_c17_file_writer` et `vitte_c17_writer_from_file` sont maintenant
  disponibles sous les mêmes noms et conservent la sémantique C : validation
  des pointeurs, écriture via `fwrite`, et flux emprunté qui n’est pas fermé.
  `writer.vitte` conserve en parallèle son API `C17Writer` avec flush et
  ownership optionnel du FILE; ce type riche reste distinct du callback
  `C17Backend.Writer`.

### Émetteurs AST et IR

- L’API AST C comprend aussi des opérations de compilation, d’objet, de
  lancement et des résultats de phase. `ast_emitter.vitte` retourne du C17
  généré (`C17AstEmitResult`) et propose une écriture vers un writer. Les
  options ont également divergé : le C expose `debug_info` et `debug_runtime`,
  tandis que Vitte expose `emit_banner` et `emit_entry_wrapper`.
- L’API IR Vitte retourne le source et une erreur textuelle. Les fonctions C
  qui appellent le compilateur hôte, créent un objet ou lancent un exécutable
  restent hors de `ir_emitter.vitte`, ce qui laisse ces opérations à la
  frontière driver/backend.
- Les sept champs de `C17IREmitOptions` correspondent aux options C de
  l’émetteur IR. Le code Vitte abaisse les instructions `phi` en affectations
  sur les arêtes de blocs; il rejette `switch`. La phrase du README qui classe
  aussi `phi` comme non pris en charge est donc à corriger.

### Données, limites et valeurs par défaut

- `module`, `program` et `translation_unit` utilisent des tableaux dynamiques
  Vitte à la place de plusieurs champs C pointeur/longueur/capacité. Les noms,
  longueurs logiques et limites principales sont présents, mais les structures
  ne sont pas interchangeables.
- Les configurations par défaut inspectées pour backend, module, naming,
  options, program, writer et unité de traduction concordent. L’étape 6 a
  aligné `emit_module_boundaries` à `true` en C et en Vitte; voir
  [`translation_unit.c`](../../src/bootstrap/backend/c17/translation_unit.c)
  et [`translation_unit.vitte`](../../src/compiler/backend/c17/translation_unit.vitte).
- Le marqueur de destruction du writer est désormais aligné sur
  `0x4445414457525452` en C/H et en Vitte. Il reste interne au cycle de vie.
- `writer.vitte` déclare `fwrite`, `fflush` et `fclose` pour son sink FILE, en
  plus de son sink natif par callback et de son sink de chemin. Le README natif
  décrit déjà ces sinks; l’écart documentaire relevé initialement est corrigé.

## Points restant à arbitrer avant la parité

1. Fixer le niveau de compatibilité attendu : parité fonctionnelle des modules
   ou remplacement de l’ABI C. Le stockage Vitte actuel correspond à une
   implémentation indépendante, pas à un drop-in binaire.
2. Définir explicitement la frontière d’exécution : les émetteurs Vitte
   produisent du C17; compilation hôte, objet et lancement relèvent du driver.

Cet audit est une inspection statique. Aucun test, build ou comparaison
différentielle n’a été lancé.

## Dépendances, erreurs, limites et cycle de vie

### Ordre des dépendances observé

L’ordre utile vient des imports, pas d’une séquence linéaire unique :

```text
backend (contrat WriteFn) ──> writer
          ├────────────────> options
          ├──> module ──> program ──> translation_unit
          └─────────────────────────> translation_unit
naming (autonome)
AST/parser/lexer ──┐
                   ├──> ast_emitter
writer ────────────┘
IR ─────────────────> ir_emitter
```

`translation_unit` importe directement `backend`, `module` et `program`.
L’émetteur AST dépend du frontend et du writer; l’émetteur IR dépend du modèle
IR et ne passe pas par le backend/writer. Le graphe n’impose donc pas que
`naming` soit initialisé avant `backend` : c’est une dépendance de politique
du compilateur appelant, pas un lien entre ces modules.

### Ordre d’utilisation et propriété mémoire

- Initialiser `writer`, puis `backend` avec son callback; appeler `backend`
  pendant sa génération; détruire le backend avant le writer. Le backend
  conserve une copie du callback et de `user_data`, sans posséder le writer ni
  son sink.
- Initialiser les modules, les enregistrer dans `program`, puis rattacher le
  programme à `translation_unit`. `program.modules[*].module` est un pointeur
  emprunté. Le programme copie le nom d’enregistrement, mais ne détruit pas le
  module. Garder chaque module vivant tant que programme et unité de traduction
  peuvent le lire.
- Préparer l’unité agrège les modules et peut finaliser/calculer l’ordre du
  programme. L’émission exige ensuite un backend en état `Generating`. La
  génération intégrée fait cet enchaînement par le callback du backend.
- Détruire/reset une unité ne détruit ni son programme ni son backend. Détruire
  un programme ne détruit pas ses modules. Ces règles sont également visibles
  dans les implémentations C/H : elles libèrent les entrées et tableaux
  possédés, pas les objets référencés.

### Échecs et états

- `backend_fail` place le backend en `Failed`; `tu_fail` place l’unité en
  `Failed`; `module_fail` enregistre l’erreur, mais ne change pas à lui seul
  l’état du module. L’émission du module a son propre chemin qui le marque
  `Failed` sur erreur de sortie. `program` n’a pas d’état : son erreur reste
  dans `last_error`.
- Les erreurs restent consultables jusqu’à l’opération qui les efface. Le
  module n’expose pas `clear_error` et conserve son erreur jusqu’à une
  initialisation, un scellement depuis `Building` ou une émission réussie. Les
  initialiseurs refusent les arguments/configurations invalides avec `false`;
  comme l’objet n’est pas encore initialisé, ils ne peuvent pas toujours
  fournir un `last_error`. Ce comportement est aussi présent dans les
  initialiseurs C/H.
- Les émetteurs AST/IR renvoient un résultat avec un texte d’erreur et une
  chaîne source. Leur API n’expose pas un état récupérable comparable à celui
  du backend; l’écriture AST vers un writer ne fait que retourner `false` si
  l’écriture échoue.

### Limites statiques relevées

| Composant Vitte | Limites/configuration observées |
| --- | --- |
| Backend | Sortie, profondeur d’indentation, identifiants et nombre d’erreurs via `BackendConfig`. |
| Writer | Sortie, lignes, colonnes, indentation et buffer; caps internes : buffer 16 MiB, capacité mémoire initiale 64 MiB, indentation 1 048 576 niveaux. |
| Naming | Identifiant source 4 096 octets; nom généré 65 536 octets. |
| Module | Nom 4 096 octets; chemin 1 MiB; les collections de fragments n’ont pas de plafond configuré dans `C17ModuleConfig`. |
| Program | 1 048 576 modules; 16 777 216 dépendances et symboles; noms 65 536 octets. |
| Translation unit | 1 048 576 inclusions et features; 16 777 216 fragments. |
| AST emitter | Profondeur 256; 512 locaux; 1 024 entrées pour chacune des tables de procédures, globales, alias et picks. |
| IR emitter | Profondeur de type 32; pas de limite de taille de sortie exposée par ses options. |

Les limites configurées du writer/backend ne bornent pas la construction des
chaînes source AST/IR : ces émetteurs assemblent leur résultat complet en
mémoire avant l’écriture éventuelle au writer. Un plafond de writer limite donc
la sortie acceptée, mais pas le pic mémoire de la phase d’émission.

### Contrat de reset du writer

`vitte_c17_writer_reset` en C/H remet à zéro le compteur et abandonne les octets
encore dans son buffer intermédiaire; pour le sink mémoire, il vide aussi les
données mémoire. Son commentaire précise que les octets déjà transmis à un
sink externe ne peuvent pas être annulés. L’étape 7 a aligné Vitte sur ce
comportement en supprimant son refus particulier pour `kind == File`. Dans les
deux versions, reset du backend et reset du writer sont séparés : le backend ne
remet pas l’état d’un sink externe à zéro.

Cet ajout reste une inspection statique des sources et headers C/H/Vitte; aucun
test ni build n’a été lancé.

## Entrées AST et IR : API contre flux réellement appelés

Les deux émetteurs Vitte natifs ont des entrées différentes et ne s’enchaînent
pas l’un vers l’autre :

| Entrée native Vitte | Précondition | Résultat |
| --- | --- | --- |
| `c17_emit_ast_with_options(parser, options)` / `c17_emit_ast(parser)` | `ParserState.Done`, racine AST de type `TranslationUnit`. | `C17AstEmitResult` contient la source C17 complète ou une erreur textuelle. |
| `c17_emit_ast_to_writer(parser, options, writer)` | Les mêmes conditions AST, puis un writer utilisable. | Écrit la chaîne générée après son assemblage complet; retourne `bool`. |
| `c17_emit_ir_with_options(context, options)` / `c17_emit_ir(context)` | `IRState.Sealed`. | `C17IRResult` contient la source C17 complète ou une erreur textuelle. |

La séparation des données est `AST -> C17` pour le chemin direct et
`AST -> HIR -> IR scellé -> C17` pour le chemin IR. L’émetteur IR Vitte ne
consomme pas l’AST de l’émetteur AST et ne fournit pas d’API writer. Les API à
résultat retournent la source; la variante AST-to-writer l’écrit elle-même.
Compilation C hôte, objet, édition de liens et exécution ne sont pas leurs
responsabilités.

Le CLI contient encore les appels C/H historiques pour le chemin sans frontend
natif. Quand le frontend natif est lié, le chemin ordinaire passe par
`pipeline.vitte` :

```text
CLI -> Vitte lexer/parser/sema -> HIR validé -> IR scellé
    -> Vitte C17 IR emitter pour le subset abaissé
    -> Vitte C17 AST emitter pour les formes non prises en charge par l'IR
    -> output C17 / compilation hôte / exécution selon les options

CLI sans frontend natif lié : parser C/H -> lowering HIR/IR C/H
                            -> émetteur IR C/H historique
```

Dans le CLI lié au frontend natif, `--emit-ir` retourne le dump de l'IR Vitte
validé. Le chemin de compilation par défaut appelle aussi le backend IR Vitte
pour les programmes que le lowering accepte. Les cas contenant des formes,
`for`, des expressions `if` à valeur ou `match` gardent l'émetteur AST Vitte
jusqu'à leur lowering IR. Le pont C/H historique conserve son chemin IR C/H
pour les builds qui ne lient pas le frontend natif.

Les appels CLI correspondants sont dans
[`execution.c`](../../src/bootstrap/cli/execution.c),
[`pipeline.c`](../../src/bootstrap/cli/pipeline.c) et le pont dans
[`native_frontend_bridge.c`](../../src/compiler/driver/native_frontend_bridge.c).
Le statut de port décrit dans
[`frontend_native_port.md`](../bootstrap/frontend_native_port.md) confirme que
ce pont et les modules Vitte C17 sont des pistes séparées.

Le test d'intégration `native_bootstrap_parity_test.py` vérifie le dump IR
natif et l'émetteur sélectionné pour une compilation scalaire par défaut ainsi
que pour un programme avec forme. Il est inclus dans `compiler-power-gate`.

## Couverture des opcodes IR

| Élément | Bootstrap C/H | Backend Vitte natif | État |
| --- | --- | --- | --- |
| phi | Abaisse les entrées sur chaque arête. | Même abaissement, corrigé pour snapshotter les entrées avant d’écrire les résultats. | Pris en charge; ce n’est pas une opération rejetée. |
| switch | Rejeté par l’émetteur. | Rejeté par l’émetteur. | Limite de l’IR : IRInstruction n’a ni table de cas ni cible par cas. |
| Constante chaîne | `IRValueKind.ConstantString`, `string_value` et constructeur `vitte_ir_const_string`. | Même représentation; émise comme littéral C17 à octets échappés. | Pris en charge. |
| Type slice | `IRTypeKind.Slice`, struct C17 avec pointeur `data` et longueur `uint64_t`. | Même représentation et déclaration de struct. | Pris en charge. |
| Opérations string/slice | Concat, longueur, sous-chaîne, création de slice, accès aux données et à la longueur. | Mêmes opérations; les helpers de concaténation/sous-chaîne sont émis à la demande. | Pris en charge. |
| Autres opcodes présents dans les deux enums | Émis ou rejetés par validation selon l’opcode. | Émis, à l’exception de `switch`. | Pas d’autre opcode commun manquant dans le dispatch inspecté. |

Les transferts phi sont des copies parallèles au sens SSA. Les deux émetteurs
écrivaient auparavant directement chaque résultat en séquence, ce qui pouvait
écraser une valeur d’entrée utilisée par un autre phi sur la même arête. Ils
snapshotent maintenant toutes les entrées dans des temporaires de portée locale,
puis écrivent les résultats.

Le README natif a été corrigé : il décrit phi comme pris en charge, explique
pourquoi switch reste rejeté, distingue l’émetteur IR de l’assemblage
module/program/unité, et documente les sinks réellement présents. La prise en
charge de switch demande d’abord d’ajouter une représentation de table de cas
à l’IR, puis de la propager à la validation, au CFG et aux émetteurs. L’étape 9
a aligné les déclarations du modèle string/slice; l’étape 10 a porté leur
émission C17.

Les sources modifiées ont été relues statiquement; aucun test ni build n’a été
lancé.

## Étape 6 — valeurs par défaut et marqueur du writer

La configuration par défaut de l’unité de traduction Vitte active maintenant
`emit_module_boundaries`, comme la configuration C/H. Le marqueur interne du
writer détruit est également identique dans les deux implémentations :
`0x4445414457525452`. Ces alignements éliminent les deux écarts de valeur
relevés plus haut; aucune vérification par build ou test n’a été effectuée.

## Étape 7 — reset des sinks writer

`vitte_c17_writer_reset` accepte maintenant aussi le sink FILE. Comme en C/H,
il vide le buffer propre au writer et réinitialise compteurs, état d’erreur,
hash et génération; la sortie mémoire est vidée, tandis que les données déjà
livrées à un callback ou à FILE restent présentes. L’état externe d’un
`C17PathSink` reste lui aussi sous le contrôle de ce sink. Vérification statique
uniquement; aucun test ni build n’a été lancé.

## Étape 8 — helpers FILE de l’API backend

`backend.vitte` expose maintenant `vitte_c17_file_writer` et
`vitte_c17_writer_from_file`. Le callback reproduit les vérifications de
pointeurs et l’écriture complète via `fwrite`; `writer_from_file` retourne un
descripteur callback qui emprunte le FILE et ne le ferme pas. Le compte des
noms présents pour `backend` passe de 67 à 69. La structure Vitte reste une
forme native; ces wrappers n’impliquent donc pas de parité binaire avec l’ABI C.
Vérification statique uniquement; aucun test ni build n’a été lancé.

## Étape 9 — déclarations du modèle IR string/slice

`IRTypeKind.Slice`, `IRValueKind.ConstantString`, le champ `string_value` des
descripteurs et valeurs, le constructeur `vitte_ir_const_string` et les six
opcodes string/slice sont déclarés dans `ir.vitte` aux positions correspondant
à C/H, alignant les valeurs enum et les formes de données.

## Étape 10 — émission C17 des chaînes et slices

L’émetteur Vitte encode les constantes chaîne en littéraux C17 à octets
échappés, déclare chaque type slice comme une struct `data`/`length`, et émet
les six opcodes. Les helpers de concaténation et sous-chaîne sont ajoutés au
source généré seulement si ces opérations apparaissent. `switch` reste la seule
opération commune explicitement non prise en charge. Vérification statique
uniquement; aucun test ni build n’a été lancé.

## Étape 17 — graphe de dépendances et choix de l’entrée

`program.add_dependency` valide dans le même ordre les identifiants nuls, les
boucles sur soi, l’existence des deux modules, les doublons et la limite de
dépendances. Selon `reject_duplicate_dependencies`, un doublon renvoie
`DuplicateDependency` ou réussit sans ajouter d’arête. Une nouvelle arête
reçoit l’indice d’enregistrement courant et invalide les résultats d’ordre et
de reachability; un doublon accepté ne modifie ni graphe ni caches. C libère
les buffers de cache à l’invalidation, tandis que Vitte vide ses tableaux.

`has_dependency` effectue la même recherche dirigée dans les deux versions :
`from_module_id -> to_module_id`. Pour un programme invalide ou une paire
absente, il renvoie `false`; il ne change ni l’erreur courante ni les caches.

`set_entry_module` exige un module enregistré, retire le marqueur d’entrée de
tous les modules, marque la cible, met à jour `entry_module_id` et invalide
l’ordre et la reachability. `clear_entry` retire tous les marqueurs, remet à
zéro `entry_module_id` et `entry_symbol_id`, désactive `has_entry` et invalide
les mêmes caches. Les deux opérations effacent l’erreur courante en cas de
succès. `set_entry_module` conserve toutefois un éventuel `entry_symbol_id`
précédent dans les deux implémentations.

Deux cas limites sont également communs : l’identifiant nul donné à
`set_entry_module` produit `UnknownModule`; et `set_entry_symbol` accepte un
symbole fonction défini de module zéro, mais laisse `entry_module_id` inchangé
(donc nul si aucun module n’avait été choisi). Si `has_entry` est vrai avec un
`entry_module_id` nul ou absent, le calcul de reachability échoue avec
`EntryNotFound`. Les deux variantes partagent ce comportement. Aucun écart de
parité n’a été relevé sur ces opérations; vérification statique uniquement,
sans test ni build.

## Étape 18 — registre et sélection des symboles du programme

`program.add_symbol` applique les mêmes contrôles logiques dans C/H et Vitte :
kind et visibilité valides, noms non vides, limites de nom et de nombre de
symboles, puis appartenance du module lorsqu’il n’est pas zéro. Le module zéro
est admis pour les symboles globaux. Un doublon signifie soit un nom généré
déjà présent dans tout le programme, soit le même nom source dans le même
module et le même kind. La configuration le rejette avec `DuplicateSymbol` ou
autorise l’ajout; dans ce dernier cas les recherches renvoient le premier
symbole enregistré. La limite du nombre de symboles est vérifiée avant le
traitement des doublons dans les deux versions.

Les identifiants de symbole commencent à 1, augmentent à chaque ajout réussi,
et l’indice d’enregistrement correspond à la position d’ajout. Les insertions
réussies effacent `last_error`; elles n’invalident pas l’ordre ni la
reachability, qui ne dépendent pas du registre de symboles. Les recherches par
ID, nom généré et tuple module/kind/nom source parcourent le registre et
renvoient le premier résultat correspondant; elles ne modifient pas l’erreur.

`set_entry_symbol` exige un symbole existant, de kind fonction et marqué comme
définition. Pour un symbole associé à un module, il appelle
`set_entry_module`, ce qui met aussi à jour les drapeaux des modules et invalide
les caches. Pour un symbole global (module zéro), il conserve le
`entry_module_id` courant, comme indiqué à l’étape 17. C/H et Vitte ont les
mêmes règles et erreurs pour ces chemins.

La différence de validation propre aux formes d’API est que Vitte peut
comparer `source_name_length` et `generated_name_length` aux longueurs des
chaînes reçues et renvoie `InvalidArgument` si l’une dépasse sa chaîne. Le C/H
reçoit des pointeurs et longueurs séparés : il vérifie pointeur non nul et
longueur non nulle, puis s’appuie sur le contrat de l’appelant pour que chaque
zone soit lisible sur la longueur annoncée. Vérification statique uniquement;
aucun test ni build n’a été lancé.

## Étape 19 — reachability, ordre et validation globale

Le calcul de reachability suit les dépendances sortantes depuis le module
d’entrée; sans entrée, tous les modules sont considérés atteignables. Le cache
des identifiants est rangé dans l’ordre d’enregistrement dans les deux
implémentations. L’ordre topologique traite `A -> B` comme « A dépend de B »,
et place donc B avant A. Avec l’ordre déterministe, les modules prêts sont
comparés octet par octet par nom, puis par ID; sinon, l’indice d’enregistrement
départage les choix. `include_unreachable_modules` décide si les modules hors
de la reachability entrent dans le tri.

Un cycle dans le sous-graphe inclus fait échouer `compute_order` avec
`DependencyCycle`. Ce rejet a lieu même si `reject_cycles` vaut `false`; le
contrôle supplémentaire du C dans `finalize` est redondant pour un programme
valide, et Vitte n’a pas ce contrôle. Avec `include_unreachable_modules=false`,
un cycle entièrement hors du sous-graphe atteignable n’empêche pas l’ordre.
Le champ `validate_modules` n’est lu ni par `finalize` ni par le validateur
dans les deux versions; la validation globale porte sur les registres et les
caches du programme, pas sur `C17Module::validate`.

Le validateur rejette les symboles dont le nom généré ou le tuple
module/kind/nom source est dupliqué, même si `reject_duplicate_symbols=false`
a permis leur insertion; `finalize` échoue alors avec `Corruption`. Ce
comportement différé est commun aux deux implémentations.

`program.validate` reste sans effet de bord et vérifie configuration, limites,
identifiants, références de modules, unicité des noms et cohérence élémentaire
des caches. Il vérifie que chaque ID d’ordre désigne un module unique et que
chaque ID de reachability désigne un module marqué atteignable; il ne prouve
pas que l’ordre est topologique ni que le cache de reachability contient tous
les modules atteignables.

Deux écarts d’implémentation ont été alignés dans cette étape. Le validateur C
contrôle maintenant que les longueurs des noms source et généré des symboles
respectent `max_name_bytes`, comme Vitte. En cas d’échec pendant le calcul de
reachability, Vitte prépare maintenant les marqueurs et la liste dans des
valeurs temporaires, puis ne les publie qu’après réussite; comme en C, les
marqueurs de module restent tous à faux et le cache reste vide si le calcul
échoue. C/H utilise une DFS récursive et Vitte une file itérative; leurs
résultats sont équivalents sur un graphe valide, mais la profondeur d’appel C
dépend de la longueur d’une chaîne de dépendances. Vérification statique;
aucun test ni build lancé.

## Étape 20 — statistiques, hash et helpers d’accès

Les statistiques concordent pour les nombres et capacités de modules,
dépendances et symboles, l’état d’ordre/reachability, la génération, l’entrée,
les octets de noms, les définitions/déclarations et les compteurs de
visibilité et de kind. Les cumuls d’octets saturent à `SIZE_MAX`/`usize.MAX`.
Le tableau `symbols_by_kind` est en `size_t`/`usize`; son initialiseur Vitte
utilise maintenant des éléments `usize`.

Le hash est le même FNV-1a 64 bits : mêmes valeurs offset/prime par défaut,
même ordre des champs, entiers encodés little-endian, booléens sur un octet et
chaînes hachées après leur longueur. Il couvre l’identité et l’ordre
d’enregistrement des modules, dépendances et symboles, ainsi que les IDs
d’entrée; il n’inclut ni capacités, caches, configuration ni génération. C/H
permet de remplacer l’offset, le prime et les capacités initiales par macros,
tandis que Vitte fixe ces constantes; l’égalité porte sur les valeurs par
défaut.

Les comptes et accès indexés renvoient zéro ou null pour programme invalide ou
indice hors limites; les versions valides ne changent pas `last_error`. Les
helpers `has_*`, getters d’IDs d’entrée et de génération, ainsi que
`has_error`, vérifient le marqueur magic comme en C/H. `last_error`, les
comptes et les accès indexés vérifient aussi la validité de base du programme;
les accesseurs sans effet de bord ne changent pas `last_error`. Les formes
retournées restent propres à chaque API : pointeur const côté C/H, valeur
optionnelle copiée côté Vitte. Vérification statique uniquement; aucun test ni
build lancé.

## Étape 21 — configuration et cycle de vie de `translation_unit`

Les valeurs par défaut de configuration, les limites, les marqueurs magic, les
variantes d’état et d’erreur concordent. La configuration active le tri,
finalize/validation du programme, validation des modules et agrégation des
includes, features, fragments et runtime; elle déduplique les includes et
features, mais pas les fragments. Les frontières de modules et l’entrée en
dernier sont activées; les sections vides sont omises. Les limites par défaut
sont 1 048 576 includes/features et 16 777 216 fragments.

`init` commence en état `Building`, génération 1, avec des agrégats vides.
`reset` efface agrégats, stats et pointeurs empruntés, revient à `Building`,
efface l’erreur et incrémente la génération jusqu’à saturation; les capacités
allouées sont conservées des deux côtés. `destroy` libère le stockage C ou
remplace les tableaux Vitte par des tableaux vides, marque l’unité détruite et
remet la génération à zéro. Les deux variantes refusent silencieusement le
reset/destroy d’un objet invalide.

Le programme et le backend restent empruntés dans les deux API. C/H copie les
chemins d’inclusion et noms de features, mais emprunte le texte des fragments
aux modules; son header exige donc que ces données source restent vivantes
jusqu’à la fin de leur utilisation. Vitte copie les chemins et noms, et
`tu_add_fragment` copie le texte dans un `string`, ce qui diffère de
l’ownership C/H. Cette distinction ne change pas la structure logique des
fragments, mais leur durée de vie n’est pas identique.

Les deux `init` écrasent l’objet après validation de la configuration. Les
déclarations C/H et Vitte indiquent maintenant qu’il faut partir d’un stockage
neuf ou détruire l’unité active avant de la réinitialiser. Les macros C/H
peuvent remplacer les marqueurs magic, les limites, les capacités initiales et
les constantes du hash; les constantes Vitte restent fixes. Vérification
statique uniquement; aucun test ni build lancé.

## Étape 22 — agrégation des données de module

La préparation parcourt les modules dans l’ordre topologique déjà disponible;
en son absence, elle parcourt l’ordre d’enregistrement et applique le même
filtre de reachability selon `include_unreachable_modules`. Dans chaque
module, les deux variantes agrègent includes, features, exigences runtime et
fragments dans le même ordre. Les exigences runtime sont combinées par OR.

La déduplication des includes compare kind et chemin; celle des features
compare le nom; celle des fragments compare section, longueur et texte. Les
doublons gardent la provenance du premier module agrégé. La déduplication
précède les vérifications de limite; une entrée dédupliquée ne consomme donc
pas de capacité. La séquence globale d’un fragment est son indice d’ajout, et
sa séquence dans le module est conservée. Le total d’octets des fragments
sature à `SIZE_MAX`/`usize.MAX`.

La validation des modules étant désactivable, deux écarts sont maintenant
couverts explicitement. C/H vérifie directement que le kind d’include est
valide, comme Vitte. Vitte utilise les longueurs déclarées des chemins,
features et fragments, plutôt que la longueur entière de la chaîne; si une
longueur dépasse la chaîne disponible, il échoue avec `InvalidModule` sans
lire hors limites. C/H reçoit pointeur et longueur séparément et reste soumis
au contrat de mémoire de l’appelant. Les cas valides ont le même contenu et le
même comportement; les chaînes dont la longueur déclarée est plus courte sont
agrégées par leur préfixe déclaré dans les deux versions.

Vitte conserve une copie du texte de chaque fragment agrégé; C/H emprunte le
buffer du module, selon la différence d’ownership déjà notée à l’étape 21.
Vérification statique uniquement; aucun test ni build lancé.

## Étape 23 — tri, préparation et statistiques d’agrégat

Quand `deterministic` est actif, les deux versions trient les includes par
kind, chemin en octets puis ID de module; les features par nom en octets puis
ID de module; les fragments par section puis séquence globale. Le tri Vitte
par insertion et le `qsort` C/H appliquent les mêmes clés. Quand le tri est
désactivé, l’ordre d’agrégation est conservé.

`prepare` accepte les états `Building` et `Prepared`, finalise le programme
selon la configuration (ou calcule son ordre s’il manque), peut le valider,
puis reconstruit les agrégats. Les deux variantes effacent agrégats et stats
avant leur reconstruction, rafraîchissent les stats après un agrégat réussi,
passent à `Prepared` et effacent l’erreur. Un échec d’agrégation laisse l’unité
en échec avec les agrégats partiels et des stats invalides; un échec antérieur
à la reconstruction laisse les anciens agrégats en place.

Les stats comptent les modules ordonnés (ou enregistrés si aucun ordre n’est
disponible), les agrégats, les octets des fragments, les exigences runtime,
la génération et les comptes/octets par section. Les additions d’octets
saturent dans les deux versions. La validation profonde Vitte refusait déjà
les comptes au-delà des limites configurées; le même contrôle est maintenant
présent en C/H. Les tris, effets d’état et stats concordent après cet
alignement. Vérification statique uniquement; aucun test ni build lancé.

## Étape 24 — émission et génération

Les deux `emit` préparent d’abord une unité `Building`, exigent ensuite l’état
`Prepared` et un backend valide, passent à `Emitting`, remettent à zéro les
compteurs d’émission et écrivent features, includes, commentaire runtime puis
fragments dans l’ordre canonique des sections. Les compteurs d’émission ne
comptent que les fragments; les octets saturent. La réussite passe à `Emitted`;
les échecs d’écriture générale signalent `Backend`, ceux des sections de
fragments `ModuleEmission`.

`generate` attache le backend puis délègue son cycle begin/callback/finish dans
les deux versions. Si le callback échoue ou si le backend ne finit pas en
`Emitted`, l’unité passe en échec; une erreur backend finale peut remplacer
l’erreur plus précise produite pendant l’émission. Les sections traitées vont
de `ForwardDeclarations` à `Epilogue`, dans le même ordre que la liste C/H; le
bandeau reste géré par le backend avant le callback.

Un écart de garde dans `emit_adapter` est corrigé : C/H vérifie maintenant la
validité du backend avant de l’attacher à l’unité, comme Vitte. Un backend
invalide ne modifie donc pas l’état, le backend associé ni l’erreur de l’unité.
La configuration `emit_entry_last` n’est lue par aucune des deux implémentations;
la section `Entry` reste toujours après les définitions de fonctions et avant
`Epilogue`. Vérification statique uniquement; aucun test ni build lancé.

## Étape 25 — writer backend et écriture d’octets

`write_n` parcourt les mêmes octets et découpe les lignes sur LF. Les deux
versions appliquent l’indentation au début de ligne si `pretty` est actif,
mettent à jour colonne, ligne et octets avec saturation, vérifient annulation,
limite de sortie et callback avant de compter les octets écrits. Un écrit de
longueur nulle ne passe pas par le callback et réussit sans exiger l’état
`Generating`; le comportement est identique. Les helpers newline, ligne vide,
indent/dedent et ouverture/fermeture de bloc ont le même ordre d’effets.

Les échappements de chaînes C concordent : guillemets, barre oblique inverse,
caractères de contrôle, octal fixe pour les autres octets non imprimables et
protection des trigraphes selon la configuration. L’écriture d’identifiants
préfixe par `vitte_`, protège les mots-clés par `kw_` et encode les octets
invalides comme `_xHH`. Le commentaire transforme LF en lignes `//` distinctes
et ignore CR dans les deux versions. Les appels C avec pointeur/longueur
restent soumis à la validité de la mémoire fournie;
les wrappers Vitte qui reçoivent une chaîne et une longueur vérifient la limite
avant de créer leurs slices. `write_c_string` reçoit un pointeur brut dans les
deux API et garde le même prérequis de mémoire valide.

Le wrapper Vitte `write_c_string` vérifie maintenant la validité du backend
avant de lire le buffer, dans le même ordre que C/H. Cela évite toute lecture
du buffer lorsque le backend est invalide. Vérification statique uniquement;
aucun test ni build lancé.

## Étape 26 — initialisation et validité des sinks writer

Les sinks callback exigent un callback d’écriture; le callback de flush reste
optionnel. Les sinks FILE exigent un stream valide et conservent le choix de
fermer ou non ce stream à la destruction. Le sink mémoire réserve au moins un
octet pour le NUL final. Vitte rejetait auparavant un callback nul après avoir
initialisé l’objet; il le refuse maintenant avant toute mutation, comme C/H.

La validité structurelle impose désormais les mêmes bornes de stockage : le
buffer de sortie d’un sink callback ou FILE ne dépasse pas la capacité
configurée, et la capacité mémoire doit dépasser sa longueur pour garder le
NUL final. Ces contrôles sont présents dans `is_valid` de chaque côté. Vitte
représente le buffer tamponné par une `string` et alloue de façon différée,
tandis que C/H réserve un buffer à l’initialisation; les points d’échec en cas
d’épuisement mémoire ne sont donc pas identiques. Le sink fichier par chemin
reste une extension Vitte sans équivalent dans l’API C/H. Vérification statique
uniquement; aucun test ni build lancé.

## Étape 27 — erreurs d’écriture et de flush des sinks

Les échecs d’écriture et de flush des sinks callback suivent le même chemin des
deux côtés : ils retournent `false`, posent l’erreur `Output` et marquent le
writer en échec. Les sinks FILE de Vitte passent par ces callbacks; ils
produisent donc la même erreur `Output`. La destruction tente le flush final
uniquement si le writer n’est ni en échec ni annulé, et retourne un échec si le
flush ou la fermeture échoue.

Un écart subsiste pour `system_error` : C/H capture `errno` après un `fwrite`
partiel ou un `fflush` en échec; Vitte ne capture pas cette valeur pour son sink
FILE, car celui-ci est implémenté comme callback booléen. L’API C/H décrit
`system_error` comme zéro lorsqu’il est indisponible ou sans objet. Aucun pont
portable vers `errno` n’existe dans le backend Vitte actuel; le champ y reste
donc à zéro dans ces cas. Les deux implémentations ne capturent pas non plus
`errno` lors d’un échec de `fclose`. Vérification statique uniquement; aucun
test ni build lancé.

## Étape 28 — accès et opérations du writer mémoire

Les getters mémoire ont les mêmes valeurs de longueur et de capacité; pour les
writers invalides ou d’un autre type, C/H retourne `NULL` pour les données et
Vitte retourne une chaîne vide, conformément à leurs types de retour. Les
opérations `reserve` et `truncate` refusent maintenant un writer structurellement
invalide sans modifier son erreur, puis signalent `NotMemoryWriter` pour un
writer valide d’un autre type, comme C/H.

`truncate` recalcule longueur écrite, position, maxima et hash. Vitte applique
maintenant aussi les limites de ligne et de colonne durant ce recalcul et
efface l’état d’échec avant de le commencer; si une limite échoue, le writer
reste en échec comme en C/H. `detach` sur un writer invalide ou non mémoire ne
modifie plus l’état ni la longueur de sortie en Vitte, ce qui suit C/H; son
résultat reste une chaîne vide plutôt qu’un pointeur nul.

La représentation et l’allocation de `detach` restent propres à chaque API :
C/H transfère le buffer alloué et peut retourner ce buffer même si l’allocation
du buffer vide de remplacement échoue; Vitte retourne une valeur `string` et
n’expose pas cet échec d’allocation de la même manière. Vérification statique
uniquement; aucun test ni build lancé.
