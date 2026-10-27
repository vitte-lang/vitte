# Vitte JSON module

Pour la documentation complète et humanisée, consulter
[DOCUMENTATION.md](DOCUMENTATION.md). Elle couvre l’installation, les imports,
le modèle `JsonValue`, le lexer, le parser, l’encodeur, les diagnostics,
l’Unicode, les nombres, la sécurité et les tests.

Le guide de consultation est aussi disponible sur le site :
[Documentation du module JSON](https://vitte-lang.org/reference/json/).
Les éditions PDF multilingues jointes à l’issue
[#17](https://github.com/vitte-lang/vitte/issues/17) sont des ressources
complémentaires ; vérifier leurs signatures et commandes contre le code de
cette révision avant de les utiliser comme référence d’API.

`json` is a Vitte source package laid out under the `json` namespace. Module
paths use `::`, as required by the Vitte grammar. For example:

```vitte
use json::error::position::{JsonPosition, start};

let origin: JsonPosition = start();
```

The source tree includes value, lexer, parser, encoder, and error/position
modules. The package currently has no native C ABI or native runtime files;
`runtime/numeric.vit` is Vitte source.

## Diagnostics for beginners

Lexer and parser results expose `diagnostic.message` for the explanation and
`diagnostic.suggestion` for an optional, beginner-friendly correction. A
suggestion is intentionally empty when the module cannot infer a safe fix.
Suggestions describe the correction; they do not modify the input
automatically. Lexical errors are copied into the parser diagnostic when
parsing starts from source text.

## Build and install

From the repository root:

```sh
make -C modules/json check
make -C modules/json test-vitte
make install PREFIX="$HOME/.local"
```

`test-vitte` stages a compiler installation, then compiles and runs a consumer
that imports the parser and encoder, parses a JSON object, and checks its
compact encoding from the installed source tree.
Normal `make install` installs the compiler and JSON source tree under
`$PREFIX/share/vitte/modules/json`; `make uninstall` removes both. A staged
installation can be made with `DESTDIR`, for example:

```sh
make install PREFIX=/usr/local DESTDIR="$PWD/modules/json/build/stage"
```

An installed compiler searches the module directory beside its own `bin`
directory. Additional module roots can be provided through `VITTE_MODULE_PATH`,
separated by the host path-list separator (`:` on Unix and `;` on Windows). This
is useful for development or a compiler invoked via a custom wrapper.

## Current compiler compatibility

The compiler parses and semantically checks every library source through
`make check-library`, including slice types, numeric `as` casts, keyword-named
`null()` procedures, and procedure prototypes. The C backend emits dynamic
arrays as owned buffers with length/capacity metadata, supports indexing,
append, and array-returning procedures, and handles nested imported spaces,
reference arguments, `string.length`, and dynamic string concatenation.

Run `make -C modules/json test-encode test-runtime test-invalid test-parse
test-roundtrip test-string-concat test-unicode` for the native integration
suites. `test-vitte` stages installation and compiles/runs a consumer importing
the installed parser and encoder API. The root `make install` and Raspberry Pi
installer both install the JSON source module through the module install target.

`package.toml` records package identity and layout. The compiler does not yet
consume this manifest or provide a general package manager.
