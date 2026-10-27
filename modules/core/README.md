# `core` — bibliothèque fondamentale de Vitte

`core` regroupe les types et opérations de base de Vitte. Ses sources sont
écrites en Vitte; aucune déclaration `extern` vers C n'y est utilisée. Les
conteneurs qui possèdent ou allouent leur stockage appartiennent au paquet
`alloc`.

## Organisation

- `types`, `traits`, `marker`, `ops` : types fondamentaux, contrats et opérateurs.
- `primitive`, `cmp`, `convert`, `num` : opérations sur les primitives et
  conversions numériques.
- `array`, `slice`, `iter`, `str` : algorithmes sur les données empruntées.
- `mem`, `ptr`, `intrinsics` : mémoire, pointeurs et opérations dépendant du
  compilateur.
- `fmt`, `hash`, `error`, `sync`, `cell`, `internal` : services fondamentaux.

## Imports

Importer les déclarations depuis leur module, par exemple :

```vit
use core::types::option::{Option, option_some, option_is_some};
use core::slice::search::{slice_find_index};
```

Les imports ne sont pas encore réexportés par le compilateur. `core::prelude`
ne constitue donc pas encore une façade pour l'ensemble du paquet.

## Contrats publics

[`API.md`](API.md) indexe les signatures de toutes les déclarations `export`;
les règles de comportement ci-dessous complètent cet index. Les signatures
Vitte constituent le contrat source. Aucun ABI binaire stable n'est promis
avant la publication d'une version majeure.

Les résultats des opérations fallibles suivent ces règles :

- les formes de conversion, parsing et arithmétique vérifiée portent
  `success`, `value` et `error`; `success == true` exige `error == None`, et
  l'échec fournit une valeur neutre (`0` ou `false`);
- `core::types::Result<T, E>` décrit le même état avec exactement une charge
  optionnelle active. `Result` ne représente pas `Some(null)` pour une valeur
  ou une erreur elle-même nullable;
- `FormatResult` porte `success`, `length` et `error`; la sortie reste dans le
  tampon fourni par l'appelant, sans terminateur ajouté, et `length` vaut zéro
  à l'échec;
- les résultats de parsing et de décodage portent `error_index`. Les parseurs
  numériques donnent l'index du caractère invalide ou du dépassement détecté;
  une conversion étroite qui dépasse sa plage signale la fin du texte. Le
  décodeur UTF-8 utilise un offset absolu en octets.

| Module | Résultat public | Erreur et cas d'échec |
| --- | --- | --- |
| `convert::cast` | `Cast*Result` | `CastError`; valeur zéro à l'échec |
| `convert::parse` | `Parse*Result` | `ParseError`, valeur neutre et `error_index` |
| `num::checked` | `CheckedI64Result`, `CheckedU64Result` | `CheckedError`, valeur zéro |
| `convert::format` | `FormatResult` | `FormatError`, `length == 0` |
| `str::utf8` | `Utf8DecodeResult` | `Utf8Error`, point de code et largeur nuls; index absolu |
| `slice::binary_search` | `BinarySearchResult` | `found == false` signifie absence, pas erreur |

`AssertionResult` transporte le code et l'emplacement fournis par l'appelant;
son champ `code` reste une donnée de diagnostic, pas une variante de `Error`.

`?T` signale une absence, une limite dépassée ou un calcul impossible selon le
module; il ne donne pas une raison détaillée. Utiliser un résultat typé lorsque
l'appelant doit distinguer ces causes. Un booléen peut représenter un prédicat,
une réussite simple ou une décision de protocole; ces usages ne sont pas
interchangeables. Par exemple, `false` de compare-exchange signifie que la
valeur observée ne correspond pas, tandis que `false` de `try_lock` signifie
que le verrou n'a pas été acquis. Les erreurs de domaine gardent leurs enums
(`CastError`, `ParseError`, `CheckedError`, `FormatError`, `Utf8Error`);
`core::error::Error` sert aux diagnostics génériques et `LocatedError` ajoute
une `SourceSpan`.

Les formes publiques gardent leurs champs spécifiques lorsqu'ils transportent
une métadonnée utile (`error_index`, `length`, `codepoint`, `width`, `span`);
elles respectent néanmoins le statut et l'erreur communs ci-dessus. Les champs
`valid` et `found` décrivent des propriétés de domaine, pas le succès d'une
opération fallible.

Le compilateur natif ne sait pas encore construire une forme générique avec une
charge `T` convertie en `?T`. `Result<T, E>` et ses accesseurs sont donc
déclarés, mais il n'existe pas encore de constructeur Vitte utilisable. Les
consommateurs doivent employer les formes concrètes de leur module
(`Cast*Result`, `Parse*Result`, `CheckedI64Result`, `FormatResult`, etc.) jusqu'à
l'ajout de cette conversion au langage. Les signatures de `TryFrom` et
`TryInto` anticipent ce type générique; leur implémentation n'est pas encore
reliée au compilateur.

`core::types::option` utilise le type nullable natif `?T`; cette représentation
ne distingue pas `None` de `Some(null)` pour un paramètre lui-même nullable.
L'extraction d'une valeur optionnelle attend une règle de déballage dans le
langage. `core::types::result` représente explicitement l'état, la valeur et
l'erreur. Les plages sont demi-ouvertes `[start, end)`. Les modules de tranches
et de chaînes travaillent sur des mémoires empruntées et n'allouent pas.

`core::cmp::partial_order` retourne `PartialOrdering`, avec `Unordered` pour
NaN. `core::cmp::total_order` place tous les NaN après les nombres et les
considère équivalents; les zéros signés sont aussi équivalents. Ce contrat ne
compare pas les bits ni les charges utiles des NaN.

Les traits de formatage écrivent dans `core::fmt::Formatter` fourni par
l'appelant, sans exiger la création d'une chaîne allouée. Les marqueurs
`Copy`, `Send`, `Sync`, `Sized` et `Unpin` sont déclarés, mais le compilateur ne
vérifie pas encore leurs implémentations ni leurs propriétés automatiques.
Les contrats de traits sont définis, mais les déclarations `impl` ne sont pas
encore reliées aux traits par l'analyse sémantique; le dispatch générique n'est
donc pas disponible.

## État d'intégration

Les opérations de layout valident les alignements et les débordements; les
tableaux utilisent une taille de pas arrondie à l'alignement de l'élément. Les
copies d'octets acceptent les zones qui se recouvrent. Les pointeurs exposent
des décalages en octets et en éléments avec contrôle du calcul d'adresse, ainsi
qu'une vérification d'alignement. Les rotations, comptages de bits, inversions
d'octets, classifications flottantes et contrôles d'ordre atomique sont écrits
en Vitte.

`MaybeUninit<T>` suit un état et une valeur nullable; ce n'est pas une réserve
d'octets réellement non initialisés. Il faut une primitive de stockage du
compilateur pour garantir cette représentation. La complétude fonctionnelle et
la compatibilité de chaque déclaration avec le compilateur restent à valider.
Ce paquet ne doit pas encore être considéré comme prêt pour une utilisation
native complète.

Les opérations dépendant de la machine sont déclarées comme intrinsics Vitte,
sans liaison C :

- `native_allocate` et `native_deallocate` pour l'allocation brute; le wrapper
  Vitte aligne les blocs décrits par `Layout` et garde l'adresse de base;
- `volatile_load` et `volatile_store` pour garantir un accès volatile unique;
- `atomic_*` pour les accès atomiques et la synchronisation;
- `trap` pour l'arrêt immédiat;
- `mem_transmute` pour les conversions de représentation.

Le backend ne les abaisse pas encore tous. Il doit fournir leur implémentation,
vérifier leurs types et leurs contraintes d'ordre mémoire. L'inventaire des
contrats, leur statut par couche et les règles d'abaissement sont dans
[`docs/compiler/intrinsic-boundaries.md`](../../docs/compiler/intrinsic-boundaries.md).
Les limites de pointeurs, d'ABI et d'atomiques par architecture sont
inventoriées dans
[`docs/compiler/target-architecture-contracts.md`](../../docs/compiler/target-architecture-contracts.md).
`core` ne reçoit pas encore un profil cible cohérent reliant ABI, endianness,
largeurs de pointeurs et capacités du backend. Enfin, les modules sont importés
par chemin complet, car la réexportation depuis les modules racine n'est pas
disponible.

Les conversions contrôlées couvrent les réductions entières signées et non
signées vers les largeurs 8, 16, 32 et 64 bits. Le parseur accepte les bases
2 à 36 et retourne aussi des résultats étroits vérifiés. Les conversions
flottant vers entier refusent NaN, les infinis et les valeurs hors plage; les
valeurs finies sont tronquées vers zéro. Les opérations
numériques 64 bits distinguent les variantes vérifiées, saturantes,
débordantes et modulo. Le formatage est sans allocation; le formatage fixe des
flottants arrondit au plus six décimales, et le formateur sait écrire un point
de code UTF-8.

Le hachage inclut FNV-1a et SipHash-2-4. Les erreurs peuvent porter une plage
source demi-ouverte et une pile fournie par l'appelant. Les wrappers atomiques
offrent charge, stockage, échange, comparaison-échange et additions/soustractions;
`Once` attend la fin de l'initialisation en cours et `SpinLock` expose une
tentative de déverrouillage. Ces APIs restent conditionnées à l'abaissement
correct des intrinsics par le backend natif.

## Suite du travail

1. Relier et abaisser toutes les intrinsics au runtime/backend natif, sans
   imports C; préserver notamment les accès `volatile` et l'alignement garanti
   par l'allocateur brut.
2. Compléter l'exécution des modules `fmt::formatter` et `fmt::padding`, puis
   vérifier la surface `core` entière avec le backend natif.
3. Vérifier les consommateurs (`alloc`, `json` et le compilateur), publier les
   réexports disponibles dans le langage et appliquer le versionnement aux
   futures ruptures de contrat.
