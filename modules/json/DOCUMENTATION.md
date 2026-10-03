# Module JSON de Vitte — documentation complète

Cette documentation explique le module `json` de Vitte de manière progressive
et détaillée. Elle commence par l’utilisation courante, puis descend jusqu’au
lexer, aux tokens, aux limites de sécurité et aux fonctions numériques.

Le module est écrit en Vitte. Il n’utilise pas de bibliothèque C externe ni de
dépendance native. Le répertoire `modules/json` contient les sources de la
bibliothèque, ses tests et son manifeste de paquet.

---

## 1. Vue d’ensemble

JSON possède six catégories de valeurs. Le module distingue en plus les deux
formes internes de nombre :

| JSON | Représentation Vitte |
|---|---|
| `null` | `JsonValueKind.Null` |
| `true` / `false` | `JsonValueKind.Bool` |
| nombre entier | `JsonValueKind.Integer` |
| nombre décimal ou exponentiel | `JsonValueKind.Float` |
| chaîne | `JsonValueKind.String` |
| tableau | `JsonValueKind.Array` |
| objet | `JsonValueKind.Object` |

Le parcours normal est :

```text
texte JSON → lexer → tokens → parser → JsonValue → encoder → texte JSON
```

Pour les usages ordinaires, travailler avec `parse`, les constructeurs de
`json::core::value` et `encode`. Les couches lexer, token et runtime numérique
sont disponibles pour les outils avancés.

---

## 2. Installation et importation

### 2.1 Vérifier et tester

Depuis la racine du dépôt :

```sh
make -C modules/json check
make -C modules/json check-library
make -C modules/json test-vitte
```

Les suites détaillées sont disponibles avec :

```sh
make -C modules/json test-encode test-runtime test-invalid test-parse \
    test-roundtrip test-string-concat test-unicode
```

### 2.2 Installer

Installation utilisateur :

```sh
make install PREFIX="$HOME/.local"
```

Installation système :

```sh
make install PREFIX=/usr/local
```

Installation dans une image ou un paquet :

```sh
make install \
    PREFIX=/usr/local \
    DESTDIR="$PWD/modules/json/build/stage"
```

Le module est installé dans :

```text
$DESTDIR/usr/local/share/vitte/modules/json
```

Le répertoire installé contient `package.toml`, `json.vit` et les sources de
la bibliothèque. Les exécutables et sorties de compilation ne font pas partie
du module livré.

### 2.3 Utiliser le module du dépôt

Pendant le développement, on peut imposer une racine de modules :

```sh
VITTE_MODULE_PATH="$PWD/modules" \
    build/bin/vitte compile application.vit -o application
```

Plusieurs racines sont séparées par `:` sous Unix :

```sh
VITTE_MODULE_PATH="$PWD/modules:$HOME/.local/share/vitte/modules" \
    build/bin/vitte compile application.vit -o application
```

### 2.4 Manifeste

`package.toml` décrit l’identité du paquet :

```toml
name = "json"
version = "0.1.0"
entry = "json.vit"
module_root = "."
module_path_syntax = "::"
native_dependencies = []
```

Les imports utilisent donc `::` :

```vitte
use json::parser::parser::*;
```

---

## 3. Organisation des espaces

| Espace | Responsabilité |
|---|---|
| `json::core::value` | arbre JSON canonique et accès génériques |
| `json::core::number` | nombres et opérations arithmétiques |
| `json::core::array` | tableau mutable et opérations de collection |
| `json::core::object` | objet, clés, membres et fusion |
| `json::lexer::lexer` | texte vers tokens |
| `json::lexer::token` | types et utilitaires des tokens |
| `json::lexer::escape` | décodage des échappements |
| `json::parser::parser` | document JSON complet |
| `json::parser::value` | valeur réutilisable dans un flux |
| `json::parser::array` | parser spécialisé de tableaux |
| `json::parser::object` | parser spécialisé d’objets |
| `json::encode::encoder` | `JsonValue` vers texte JSON |
| `json::encode::escape` | échappement, citation et métriques |
| `json::error::position` | offsets, lignes, colonnes et spans |
| `json::error::error` | modèle commun d’erreur |
| `json::runtime::numeric` | conversions et formatage des nombres |

Pour une application, importer généralement :

```vitte
use json::core::value::*;
use json::parser::parser::*;
use json::encode::encoder::*;
```

Les imports sélectifs sont préférables dans une grande application :

```vitte
use json::core::value::{JsonValue, JsonValueKind, string, object};
use json::parser::parser::{JsonParseResult, parse_strict};
use json::encode::encoder::{JsonEncodeResult, encode_pretty};
```

---

## 4. Premier exemple complet

```vitte
use json::parser::parser::{JsonParseResult, parse_strict};
use json::encode::encoder::{JsonEncodeResult, encode_pretty};

proc convert(source: string) {
    let parsed: JsonParseResult = parse_strict(source);

    if !parsed.ok {
        // parsed.diagnostic contient l’erreur détaillée.
        give;
    }

    let encoded: JsonEncodeResult = encode_pretty(&parsed.value);

    if !encoded.ok {
        // La valeur doit être validée avant de devenir une sortie.
        give;
    }

    // encoded.text contient le document JSON formaté.
    give;
}
```

La règle fondamentale est de tester `ok` avant de lire la valeur produite.
En cas d’échec, le contenu par défaut de `value` ou `text` ne doit pas être
considéré comme une valeur métier.

---

## 5. Le modèle `JsonValue`

### 5.1 Définition

Le type central est :

```vitte
export form JsonValue {
    kind: JsonValueKind,
    bool_value: bool,
    integer_value: i64,
    float_value: f64,
    string_value: string,
    array_value: [JsonValue],
    object_value: [JsonMember],
    span: JsonSpan,
}

export form JsonMember {
    key: string,
    value: JsonValue,
}
```

`JsonValue` est une union manuelle : `kind` indique quel champ est pertinent.
Il faut donc consulter `kind` avant d’utiliser un champ de contenu.

`JsonValueKind` contient :

```text
Null, Bool, Integer, Float, String, Array, Object
```

`JsonValueError` contient notamment :

```text
None, TypeMismatch, IndexOutOfBounds, KeyNotFound, DuplicateKey,
EmptyKey, InvalidValue, InvalidNumber, InvalidConversion, Overflow,
Underflow, DepthExceeded
```

### 5.2 Construire des valeurs

```vitte
let nothing: JsonValue = null();
let active: JsonValue = boolean(true);
let answer: JsonValue = integer(42);
let temperature: JsonValue = floating(21.5);
let title: JsonValue = string("Vitte");

let empty_list: JsonValue = empty_array();
let empty_map: JsonValue = empty_object();
```

`null_value()` est l’alias explicite de `null()`.

Construire un objet :

```vitte
let name_member: JsonMember = member("name", string("Vitte"));
let version_member: JsonMember = member("version", integer(1));

let package_value: JsonValue = object([
    name_member,
    version_member
]);
```

Construire un tableau imbriqué :

```vitte
let first: JsonValue = integer(1);
let second: JsonValue = integer(2);
let numbers: JsonValue = array([first, second]);

let wrapper: JsonValue = object([
    member("numbers", numbers),
    member("valid", boolean(true))
]);
```

### 5.3 Tester le type

Fonctions disponibles :

```text
is_null, is_bool, is_integer, is_float,
is_number, is_string, is_array, is_object
```

Exemple :

```vitte
if is_string(&value) {
    let text: string = value.string_value;
}

if is_number(&value) {
    // vrai pour Integer et Float
}
```

### 5.4 Lire avec une valeur de remplacement

Les helpers suivants évitent une erreur lorsqu’un champ est facultatif :

```text
bool_or, integer_or, float_or, number_or, string_or
```

```vitte
let host: string = string_or(&value, "localhost");
let port: i64 = integer_or(&value, 8080);
let debug: bool = bool_or(&value, false);
```

Cette stratégie est pratique pour une configuration tolérante. Pour une API
stricte, tester le type et produire un diagnostic métier au lieu d’accepter
silencieusement une valeur incorrecte.

### 5.5 Tableaux

L’API générique exporte :

```text
array_len(value)
array_get_ref(value, index)
```

```vitte
let size: usize = array_len(&value);
if size > 0 {
    let first: &JsonValue = array_get_ref(&value, 0);
}
```

`json::core::array` fournit la couche complète : capacité, réservation,
ajout, ajout en tête, insertion, remplacement, suppression, `pop`, tranches,
concaténation, répétition, inversion, rotation, recherche, comptage,
déduplication, aplatissement, sommes et minimum/maximum.

### 5.6 Objets

L’API générique exporte :

```text
object_len(value)
object_get_ref(value, key)
object_member_ref(value, index)
```

```vitte
let title: &JsonValue =
    object_get_ref(&package_value, "name");
```

`json::core::object` propose en plus l’insertion, le remplacement, le
renommage, la suppression, l’accès au premier et au dernier membre, les listes
`keys`, `values`, `members`, la fusion, l’intersection et la différence.

Les membres sont conservés dans l’ordre de lecture. Cependant, l’égalité
logique d’un objet ne dépend pas de l’ordre des membres ; l’ordre des tableaux,
lui, est significatif.

### 5.7 Comparaison et inspection

```vitte
let same: bool = equals(&left, &right);
let nesting: usize = depth(&value);
let total: usize = total_value_count(&value);
let duplicated: bool = has_duplicate_keys(&value);
let valid: bool = validate_json_value(&value);
```

Les valeurs scalaires ont une profondeur nulle. Chaque conteneur imbriqué
augmente la profondeur. `total_value_count` inclut les conteneurs eux-mêmes.

---

## 6. Parser complet

Le parser principal se trouve dans `json::parser::parser`.

### 6.1 API

```text
parse(source)
parse_strict(source)
parse_tokens(tokens)
parse_tokens_strict(tokens)
```

`parse` effectue le lexing puis le parsing d’un document complet.
`parse_strict` utilise la même grammaire et rejette les clés dupliquées.
Les variantes `parse_tokens` servent lorsqu’une application possède déjà les
tokens, par exemple après une phase d’inspection ou de cache.

### 6.2 Grammaire reconnue

```text
json   ::= value EOF
value  ::= null | boolean | number | string | array | object
array  ::= "[" "]" | "[" value { "," value } "]"
object ::= "{" "}" | "{" member { "," member } "}"
member ::= string ":" value
```

Documents valides :

```json
null
true
42
-3.14
1e-3
"texte"
[]
{}
[1, "deux", null]
{"name": "Vitte", "active": true}
```

Le JSON standard n’autorise ni commentaires, ni clés non citées, ni
guillemets simples, ni virgules finales.

### 6.3 Résultat

```vitte
export form JsonParseResult {
    ok: bool,
    value: JsonValue,
    next: usize,
    diagnostic: JsonParserDiagnostic,
    statistics: JsonParserStatistics,
}
```

`ok` doit être vérifié en premier. `value` est l’arbre racine en cas de
succès. `next` indique la progression dans le flux de tokens. `statistics`
contient le nombre de tokens, de valeurs, de scalaires, d’objets, de tableaux,
de membres et la profondeur maximale observée.

Le parser de document exige une valeur puis `End`. Une seconde valeur racine
produit `TrailingToken`.

### 6.4 Clés dupliquées

La politique est représentée par :

```text
Allow
Reject
KeepFirst
KeepLast
```

`parse_strict` choisit `Reject`. Le document suivant est donc refusé en mode
strict :

```json
{"role": "user", "role": "admin"}
```

Cette décision est importante pour la sécurité : deux logiciels peuvent sinon
retenir deux occurrences différentes d’une même clé.

---

## 7. Diagnostics du parser

```vitte
export form JsonParserDiagnostic {
    error: JsonParserError,
    phase: JsonParserPhase,
    code: string,
    message: string,
    detail: string,
    suggestion: string,
    expected: string,
    found: string,
    key: string,
    span: JsonSpan,
    related_span: JsonSpan,
    has_related_span: bool,
}
```

Les erreurs comprennent notamment `EmptyDocument`, `UnexpectedEnd`,
`UnexpectedToken`, `TrailingToken`, `InvalidNull`, `InvalidBoolean`,
`InvalidNumber`, `InvalidString`, les erreurs de séparateur des tableaux et
objets, `DuplicateObjectKey`, `TooManyValues`, `DepthExceeded` et
`InvalidTokenStream`.

`phase` vaut `None`, `Document`, `Value`, `Array` ou `Object`.

- `code` est adapté aux tests et aux outils ;
- `message` est destiné à l’utilisateur ;
- `detail` ajoute un contexte ;
- `expected` et `found` permettent une présentation personnalisée ;
- `key` identifie une clé concernée ;
- `span` localise l’erreur ;
- `related_span` peut montrer une occurrence liée ;
- `suggestion` propose parfois une correction, sans modifier la source.

Exemple d’utilisation :

```vitte
use json::error::position::{human_line, human_column};

let result: JsonParseResult = parse(source);
if !result.ok {
    let line: usize = human_line(&result.diagnostic.span.start);
    let column: usize = human_column(&result.diagnostic.span.start);
    // Afficher code, message, suggestion, line et column.
    _ = line;
    _ = column;
}
```

---

## 8. Positions et spans

`json::error::position` définit :

```vitte
export form JsonPosition {
    offset: usize,
    line: usize,
    column: usize,
}

export form JsonSpan {
    start: JsonPosition,
    end: JsonPosition,
}
```

Les coordonnées internes sont zéro-based. La première ligne et la première
colonne valent donc zéro. Pour un affichage humain, utiliser
`human_line` et `human_column`, qui ajoutent un.

Fonctions de construction et de progression :

```text
position, start, origin, at_offset
advance_column, advance_columns, advance_line
advance_lf, advance_cr, advance_crlf
advance_character, advance_text
with_offset, with_line, with_column
position_after
```

Fonctions de comparaison :

```text
equal, same_offset, before, before_or_equal,
after, after_or_equal, compare, minimum, maximum,
distance, same_line, line_distance
```

Fonctions sur les spans :

```text
span, point, empty_span, span_from_start,
span_from_text, span_for_text, span_valid, span_empty,
span_length, span_single_line, span_multiline, span_line_count,
spans_equal, span_before, span_after,
span_contains_position, span_contains_position_inclusive,
span_contains_span, spans_overlap, spans_touch,
spans_overlap_or_touch, merge_spans, span_between
```

Les spans permettent à un éditeur ou à un compilateur de signaler précisément
une erreur sans rescanner le texte complet.

---

## 9. Lexer et tokens

Le lexer transforme le texte en `JsonLexerResult`.

### 9.1 Types

`JsonTokenKind` contient :

```text
Invalid, End,
LeftBrace, RightBrace, LeftBracket, RightBracket, Colon, Comma,
Null, True, False, Number, String
```

`JsonToken` est :

```vitte
export form JsonToken {
    kind: JsonTokenKind,
    lexeme: string,
    value: string,
    span: JsonSpan,
}
```

`lexeme` conserve le texte original. `value` contient la valeur décodée quand
elle existe, particulièrement pour les chaînes. Le lexer émet un token `End`
final et ne représente pas les espaces comme des tokens.

### 9.2 Utilisation

```vitte
use json::lexer::lexer::{JsonLexerResult, lex};

let result: JsonLexerResult = lex("{\"answer\": 42}");
if result.ok {
    let tokens: [JsonToken] = result.tokens;
}
```

Le module `json::lexer::token` fournit aussi les constructeurs
`invalid`, `end`, `punctuation`, `literal`, `string_token`, `number_token` et
les fonctions de nommage et de comparaison.

### 9.3 Limites du lexer

| Limite | Valeur par défaut |
|---|---:|
| tokens | `1 048 576` |
| longueur d’une chaîne | `16 777 216` |
| longueur d’un nombre | `4 096` |
| longueur de la source | `268 435 456` |

Les limites évitent qu’un document excessif ne consomme toutes les ressources
de l’application. `JsonLexerLimits` permet de les réduire pour un protocole
ou une configuration contrôlée.

---

## 10. Chaînes et Unicode

Le lexer reconnaît les échappements JSON standard :

```text
\"  guillemet       \\  antislash
\/  slash           \b  retour arrière
\f  saut de page    \n  nouvelle ligne
\r  retour chariot  \t  tabulation
\uXXXX code UTF-16 hexadécimal
```

Exemple :

```json
{"message": "ligne 1\nligne 2", "heart": "\u2764"}
```

Les caractères hors BMP peuvent utiliser une paire UTF-16 :

```json
"\uD83D\uDE80"
```

Le module vérifie les surrogates hauts et bas, compose le scalaire Unicode,
refuse les surrogates isolés et convertit le résultat en UTF-8.

`json::lexer::escape` propose :

```text
decode_escape(source, index)
escape_text(source)
escape_error_name(error)
```

`decode_escape` reçoit l’index du caractère `\\`. Ces fonctions travaillent sur
le contenu échappé ; le parser complet, lui, gère aussi les guillemets et les
spans de la chaîne.

---

## 11. Encodeur

Le module `json::encode::encoder` produit du texte JSON à partir d’un
`JsonValue`.

### 11.1 Résultat et erreurs

```vitte
export form JsonEncodeResult {
    ok: bool,
    text: string,
    error: JsonEncodeError,
}
```

`JsonEncodeError` contient `None`, `InvalidValue`, `InvalidNumber`,
`InvalidString`, `DuplicateKey` et `DepthExceeded`.

### 11.2 Fonctions

```text
encode(value)
encode_compact(value)
encode_pretty(value)
encode_pretty_with_indent(value, indent)
encode_with_options(value, options)
```

```vitte
let compact: JsonEncodeResult = encode_compact(&value);
let readable: JsonEncodeResult = encode_pretty(&value);
let four_spaces: JsonEncodeResult =
    encode_pretty_with_indent(&value, 4);
```

`encode` et `encode_compact` produisent une sortie compacte. `encode_pretty`
utilise deux espaces par niveau. Le résultat d’un pretty print peut être
réindenté avec `encode_pretty_with_indent`.

### 11.3 Options

```vitte
export form JsonEncodeOptions {
    pretty: bool,
    indent: usize,
    escape_slash: bool,
}
```

```vitte
let options: JsonEncodeOptions = JsonEncodeOptions {
    pretty: true,
    indent: 2,
    escape_slash: false,
};

let result: JsonEncodeResult =
    encode_with_options(&value, options);
```

`escape_slash` contrôle si `/` devient `\/`. Les deux représentations sont
valides JSON.

### 11.4 Helpers scalaires

```text
encode_null()
encode_bool(value)
encode_integer(value)
encode_float(value)
encode_string(value)
```

Ils construisent temporairement un `JsonValue`, puis utilisent le même chemin
de validation et d’encodage que l’API générale.

L’encodeur refuse les valeurs incohérentes, les nombres non finis, les
chaînes invalides, les objets à clés dupliquées et les arbres trop profonds.

---

## 12. Échappement indépendant

`json::encode::escape` ne sérialise pas un arbre. Il travaille uniquement sur
des chaînes.

Profils disponibles :

```text
default_options()
ascii_options()
slash_options()
strict_ascii_options()
```

Fonctions d’échappement sans guillemets externes :

```text
escape(source)
escape_ascii(source)
escape_slashes(source)
escape_strict_ascii(source)
```

Fonctions produisant une chaîne JSON complète :

```text
quote(source)
quote_ascii(source)
quote_slashes(source)
quote_strict_ascii(source)
```

Exemple conceptuel :

```text
source : bonjour "Vitte"
escape: bonjour \"Vitte\"
quote : "bonjour \"Vitte\""
```

Pour inspecter une chaîne avant l’encodage, utiliser `validate_string`,
`requires_escaping`, `requires_escaping_with_options`, `escape_count` et
`statistics`. Pour prévoir la mémoire nécessaire, utiliser `escaped_length` et
`quoted_length`.

---

## 13. Nombres JSON

### 13.1 Grammaire

```text
number = [ "-" ] int [ frac ] [ exp ]
int    = "0" | digit1-9 *digit
frac   = "." 1*digit
exp    = ("e" | "E") ["-" | "+"] 1*digit
```

Valides : `0`, `-0`, `12`, `-12`, `3.14`, `10e3`, `10E+3`, `10e-3`.

Invalides : `+1`, `01`, `1.`, `.5`, `1e`, `NaN`, `Infinity`.

### 13.2 Runtime numérique

`json::runtime::numeric` sépare validation, conversion, arithmétique et
formatage. Les types principaux sont :

```text
JsonNumericKind  = Invalid | Integer | Float
JsonNumericClass = Invalid | Zero | Positive | Negative
```

Les erreurs couvrent les caractères invalides, signes incorrects, zéros
initiaux, parties fractionnaires ou exposants incomplets, débordements,
sous-débordements, conversions négatives vers `u64`, divisions par zéro,
exposants trop grands et valeurs non finies.

### 13.3 Fonctions publiques

```text
scan(source)
valid(source)
classify(source)
parse_u64(source)
parse_i64(source)
parse_f64(source)
format_u64(value)
format_i64(value)
format_f64(value)
normalize(source)
```

Le scanner conserve les bornes des parties entière, fractionnaire et
exponentielle. `parse_i64` et `parse_u64` refusent les dépassements au lieu de
tronquer. `parse_f64` refuse les résultats non finis.

Les constantes exportées sont `JSON_NUMERIC_I64_MAX` et
`JSON_NUMERIC_I64_MIN`.

---

## 14. Modèle commun d’erreur

`json::error::error` fournit un modèle utilisable par une application qui veut
uniformiser les erreurs du lexer, du parser, du décodeur, de l’encodeur et du
validateur.

Il décrit :

- `JsonErrorKind` ;
- `JsonErrorPhase` ;
- `JsonErrorSeverity` ;
- un code stable ;
- message et détail ;
- texte attendu et trouvé ;
- source et span ;
- cause éventuelle.

Les constructeurs couvrent notamment `unexpected_character`, `unexpected_token`,
`unexpected_end`, `invalid_literal`, `invalid_number`, `invalid_string`,
`invalid_escape`, `invalid_unicode_escape`, `invalid_unicode_scalar`,
`invalid_surrogate`, `missing_colon`, `missing_comma`, `missing_value`,
`missing_key`, `trailing_comma`, `duplicate_key`, `type_mismatch`,
`index_out_of_bounds`, `key_not_found`, `conversion_error`, `depth_exceeded`,
`encoding_error`, `decoding_error`, `validation_error` et `internal_error`.

---

## 15. Limites et sécurité

Les limites par défaut du parser sont :

| Limite | Valeur |
|---|---:|
| profondeur maximale | `512` |
| valeurs totales | `1 048 576` |
| éléments de tableaux | `1 048 576` |
| membres d’objets | `1 048 576` |
| longueur d’une clé | `16 777 216` |

Le lexer ajoute ses limites de source, tokens, chaînes et nombres. Toutes ces
valeurs peuvent être réduites pour une API exposée sur Internet ou un fichier
de configuration local.

Recommandations pour des données non fiables :

1. utiliser `parse_strict` ;
2. réduire les limites au besoin ;
3. vérifier `result.ok` ;
4. vérifier les types et les plages métier ;
5. valider avant l’encodage ;
6. refuser les clés dupliquées ;
7. ne pas appliquer automatiquement les suggestions ;
8. ne pas supposer que l’ordre des clés est une sémantique métier ;
9. considérer les erreurs numériques comme importantes.

---

## 16. Cas d’utilisation

### 16.1 Configuration

```vitte
let result: JsonParseResult = parse_strict(source);
if !result.ok {
    give;
}

if !is_object(&result.value) {
    give;
}

let host: string =
    string_or(&object_get_ref(&result.value, "host"), "localhost");
let port: i64 =
    integer_or(&object_get_ref(&result.value, "port"), 8080);
```

Pour une configuration critique, vérifier aussi les clés inconnues, les plages
de nombres et les champs obligatoires.

### 16.2 API HTTP

Requête :

```text
octets HTTP → chaîne UTF-8 → parse_strict → validation métier
```

Réponse :

```text
valeur métier → JsonValue → validate → encode_compact → corps HTTP
```

Le module JSON ne décide pas du code HTTP, de l’authentification, des headers
ou de la taille maximale d’une requête : ces responsabilités sont celles de
la couche réseau.

### 16.3 Transformation

`JsonValue` est excellent pour modifier les valeurs, mais ne conserve pas les
espaces ou commentaires de présentation. Pour un formateur qui doit préserver
la source, conserver aussi les tokens et leurs spans.

### 16.4 Validateur métier

Le module n’est pas un moteur JSON Schema. Il fournit cependant les briques
nécessaires : tests de type, accès aux clés et indices, compteurs, profondeur,
détection des doublons et positions précises.

Toujours séparer :

1. JSON syntaxiquement invalide ;
2. JSON valide mais de mauvais type ;
3. JSON valide et correctement typé mais interdit par le métier.

---

## 17. Pièges fréquents

### Confondre lexer et parser

`lex` produit des tokens. `parse` construit l’arbre et vérifie les relations
entre les tokens.

### Lire un champ sans vérifier `kind`

Les champs de `JsonValue` ne sont pas des conversions automatiques. Utiliser
les prédicats ou les helpers `*_or`.

### Oublier la fin du document

Le parser complet attend une valeur puis `End`. Une deuxième valeur racine est
une erreur.

### Croire qu’un objet est ordonné

L’ordre est conservé pour la sortie et l’inspection, mais l’égalité logique
des objets n’en dépend pas.

### Transformer tous les nombres en `f64`

Cela peut perdre la précision des grands entiers. Le module conserve les
entiers dans `i64` lorsque cela est possible.

### Confondre chaîne brute et chaîne échappée

Le texte du token (`lexeme`) et sa valeur décodée (`value`) peuvent différer.

### Accepter les commentaires

Les commentaires ne font pas partie de JSON standard et sont refusés.

---

## 18. Tests et maintenance

Commandes utiles :

```sh
make -C modules/json check
make -C modules/json check-library
make -C modules/json check-tests
make -C modules/json test-runtime
make -C modules/json test-encode
make -C modules/json test-invalid
make -C modules/json test-parse
make -C modules/json test-roundtrip
make -C modules/json test-string-concat
make -C modules/json test-unicode
make -C modules/json test-vitte
make -C modules/json verify
```

| Fichier | Couverture |
|---|---|
| `tests/encode.vit` | encodeur |
| `tests/invalid.vit` | entrées invalides |
| `tests/parse.vit` | parser et diagnostics |
| `tests/roundtrip.vit` | parse puis encode |
| `tests/string_concat.vit` | chaînes |
| `tests/unicode.vit` | Unicode |
| `runtime/test_numeric.vit` | runtime numérique |
| `tests/import_smoke.vit` | import installé |
| `encode/escape.vit` | échappement détaillé |
| `core/array.vit` | opérations de tableau |
| `core/object.vit` | opérations d’objet |
| `core/number.vit` | nombres |

Pour ajouter une fonction publique : choisir le bon espace, documenter le
contrat, représenter explicitement les erreurs, tester le cas nominal et les
cas limites, puis lancer `make -C modules/json verify`.

---

## 19. Référence rapide

### Parser

```text
json::parser::parser::parse
json::parser::parser::parse_strict
json::parser::parser::parse_tokens
json::parser::parser::parse_tokens_strict
```

### Valeurs

```text
json::core::value::null
json::core::value::boolean
json::core::value::integer
json::core::value::floating
json::core::value::string
json::core::value::array
json::core::value::object
json::core::value::is_null
json::core::value::is_bool
json::core::value::is_integer
json::core::value::is_float
json::core::value::is_number
json::core::value::is_string
json::core::value::is_array
json::core::value::is_object
json::core::value::equals
json::core::value::depth
json::core::value::validate
```

### Encodeur

```text
json::encode::encoder::encode
json::encode::encoder::encode_compact
json::encode::encoder::encode_pretty
json::encode::encoder::encode_pretty_with_indent
json::encode::encoder::encode_with_options
json::encode::encoder::encode_null
json::encode::encoder::encode_bool
json::encode::encoder::encode_integer
json::encode::encoder::encode_float
json::encode::encoder::encode_string
```

### Lexer et Unicode

```text
json::lexer::lexer::lex
json::lexer::token::JsonToken
json::lexer::token::JsonTokenKind
json::lexer::escape::decode_escape
json::lexer::escape::escape_text
```

### Positions

```text
json::error::position::JsonPosition
json::error::position::JsonSpan
json::error::position::human_line
json::error::position::human_column
json::error::position::span_length
json::error::position::merge_spans
```

### Numérique

```text
json::runtime::numeric::scan
json::runtime::numeric::valid
json::runtime::numeric::classify
json::runtime::numeric::parse_u64
json::runtime::numeric::parse_i64
json::runtime::numeric::parse_f64
json::runtime::numeric::format_u64
json::runtime::numeric::format_i64
json::runtime::numeric::format_f64
json::runtime::numeric::normalize
```

---

## 20. Philosophie et parcours recommandé

Le module suit ces principes :

- distinguer les valeurs et leur représentation textuelle ;
- conserver les informations utiles du document source ;
- préférer une erreur explicite à une conversion silencieuse ;
- rendre les limites configurables ;
- fournir une API simple et des couches bas niveau réutilisables ;
- traiter Unicode et les nombres comme des sujets de correction ;
- tester continuellement le round-trip parse/encode.

Pour une application courante :

```text
parse_strict → JsonValue → validation métier → encode_compact/pretty
```

Pour un outil avancé :

```text
lex → tokens → parse_tokens → JsonValue
```

En conservant les `JsonSpan`, un éditeur ou un compilateur peut signaler une
erreur au bon endroit et relier plusieurs occurrences d’un même problème.

Le réflexe à retenir est simple : parser explicitement, vérifier le résultat,
inspecter les types, valider les contraintes métier et vérifier le résultat de
l’encodage avant d’utiliser le texte produit.
