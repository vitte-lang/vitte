# Vitte 0.1.1 : périmètre vérifié

La grammaire canonique est [`grammar/vitte.ebnf`](../../grammar/vitte.ebnf). La matrice de référence, qui sépare parsing, analyse, génération C17 et exécution pour les deux chemins du compilateur, se trouve dans [État du support](../../docs/reference/support.html). Le triage des bugs, plateformes et commandes de la version 0.1.1 est dans [l’audit de publication](compiler_0.1.1_audit.md).

Ce document remplace l’ancienne matrice, qui pointait vers `src/vitte/grammar` et décrivait un compilateur sans HIR, IR ni backend C17. Ces affirmations ne correspondent plus au dépôt.

## Syntaxe et comportement

La grammaire décrit les formes reconnues, pas une garantie de compilation. Pour une construction donnée, la matrice indique quel frontend et quelles phases ont une preuve de test. Le mot « disponible » désigne seulement les cas exécutés par les fixtures cités ; il ne transforme pas toute la grammaire en promesse de compatibilité.

Les cas disponibles couvrent notamment les procédures, constantes et statiques, alias, formes et choix, imports, génériques et implémentations testés, tableaux, opérateurs, contrôle de flux, `match`, contrats, `defer`, `assert` et génération C17. Les tâches `async`/`await` n’offrent qu’une exécution différée puis synchrone au premier `await`; les macros et déplacements partiels restent expérimentaux. Consultez la ligne exacte de chaque construction avant de l’utiliser.

Pour les choix (`pick`), les constructeurs de valeurs et les motifs de branche `match` s’écrivent tous deux `State::Ready`. Le test [native_frontend_subset_test.py](../tests/native_frontend_subset_test.py) couvre cette syntaxe. La grammaire exige les parenthèses dans `assert(condition);`; `assert condition;` n’est pas une syntaxe Vitte prise en charge.

## Limites à conserver dans la documentation

- Une forme présente dans l’EBNF peut manquer dans un frontend, une phase ou un backend. Ne déduisez pas le support d’un parsing réussi.
- Les tâches async s’exécutent de façon différée puis synchrone au premier `await`. Vitte 0.1.1 ne fournit pas d’ordonnanceur général, de `spawn`/`yield` ni de concurrence.
- `unsafe` ne vérifie pas la sûreté mémoire ; les durées de vie et l’exclusivité des références ne sont pas prouvées complètement.
- Certaines formes avancées restent expérimentales, notamment les macros et les déplacements partiels ou indirects. La matrice précise chaque sous-cas.
- La commande CLI `vitte test` et le formateur `vitte fmt` sont reconnus par le parseur, mais ne sont pas exécutés par le dispatcher natif de 0.1.1. Voir le suivi [#45](https://github.com/vitte-lang/vitte/issues/45).
- Le paquet `json` 0.1.1 s’installe, mais son code contient des assertions sans parenthèses et échoue donc à l’analyse du compilateur. Ce paquet n’est pas une preuve de compatibilité langage ; voir [#46](https://github.com/vitte-lang/vitte/issues/46).

## Plateformes et commandes

Les seules cibles de paquets 0.1.1 déclarées par la CI sont Linux x86_64, macOS arm64 et Windows x86_64/MSYS2 UCRT64. La publication reste bloquée tant que leurs gates de build, de self-host et d’installation ne sont pas toutes vertes. Les builds d’architectures 32 bits, embarquées, BSD, Solaris et illumos sont des expériences de portabilité, pas des plateformes officiellement supportées.

Le seed C se construit avec `make bootstrap`; `make bootstrap-check` vérifie le seed et ses diagnostics; `make` construit le binaire principal; `make MODE=release bootstrap-selfhost` vérifie les stages 0 à 3. Le contrat complet et l’état de chaque gate figurent dans [l’audit 0.1.1](compiler_0.1.1_audit.md). Les toolchains C doivent accepter C17. Les workflows utilisent GCC sur Linux, le `cc`/Clang fourni avec Xcode sur macOS et MinGW-w64 GCC dans MSYS2 UCRT64 sur Windows ; leurs versions ne sont pas épinglées.
