# Compilateur Vitte

[![Vitte CI](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml/badge.svg)](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml)

Ce dépôt contient le compilateur officiel de Vitte. Le seed est en C17 ; le
binaire principal utilise un frontend écrit en Vitte et quelques adaptateurs C
pour l’OS et les outils hôtes. Le pipeline du compilateur est :

```text
source -> lexer -> parser/AST -> semantic analysis -> HIR -> IR -> C17
```

## Build

Prérequis : un compilateur C17 (GCC ou Clang), Steel et Python 3 pour les
vérifications. Make choisit `cc`, puis `clang`, puis `gcc` si `CC` n’est pas
explicitement défini. Pour imposer un compilateur, utilisez par exemple
`make CC=clang`. Le profil debug utilise `-O2` avec les symboles de débogage,
afin que le seed reste réactif lors de la génération du frontend natif.
Le fichier de build natif recommandé est [steelconf](steelconf), écrit en
MUF4 (`!muf 4`). Le Makefile historique reste documenté pendant la migration,
mais les nouvelles intégrations doivent cibler Steelconf.

```sh
cc --version
steel --version
python3 --version
```

### Build natif Steelconf

Depuis la racine de Vitte :

```sh
steel run --file steelconf
steel run --file steelconf --bake bootstrap
steel run --file steelconf --jobs 4
steel run --file steelconf --bake check
steel run --file steelconf --bake test
```

Les équivalents des opérations usuelles sont également disponibles :

```sh
steel clean --file steelconf
steel test --file steelconf --jobs 4
steel install --file steelconf
```

Les variables de compatibilité Make peuvent être fournies par l’environnement,
par exemple `FILE=examples/hello.vit steel run --file steelconf --bake
compile`. Les cibles sont des bakes MUF4 ; leurs dépendances, sorties, profils
et recettes sont donc validés avant exécution.

### Seed C : `vitte-bootstrap`

`vitte-bootstrap` est le compilateur seed. Make le construit depuis les sources
C17 du dépôt. Il sert ensuite à compiler l’API du frontend natif, écrite en
Vitte, vers une unité C.

```sh
make bootstrap
./build/bin/vitte-bootstrap --version
```

À la première construction, Make affiche notamment :

```text
Built Vitte bootstrap compiler:
  build/bin/vitte-bootstrap
Bootstrap compiler ready: build/bin/vitte-bootstrap
vitte <version indiquée dans VERSION>
```

Sous Windows avec MSYS, `make bootstrap` construit également le seed ; son
exécutable s’appelle `build/bin/vitte-bootstrap.exe`.
Le séparateur des racines `VITTE_MODULE_PATH` est déterminé depuis la cible du
compilateur, pour rester cohérent avec l’environnement MSYS ou MinGW utilisé.

### Frontend natif : `vitte`

La construction normale lance le seed si nécessaire, génère le C de l’API
native, puis lie `build/bin/vitte` :

```sh
make
./build/bin/vitte --version
```

La génération de l’API affiche les phases du compilateur seed pour distinguer
une phase lente d’un build bloqué.

La commande affiche `vitte` suivi de la version inscrite dans `VERSION`.

Les deux exécutables ont des chemins distincts. Utilisez le seed pour examiner
ou déboguer l’étape initiale ; utilisez `vitte` pour le frontend natif habituel.
Les commandes acceptées et les constructions compilées dépendent du frontend
et du support courant ; voir la [matrice de support](docs/reference/support.html).

### Installer Vitte

Sous Windows, depuis le terminal MSYS2 du dépôt, lancez `make install-windows`.
Le compilateur est installé dans `%LOCALAPPDATA%\Programs\Vitte\bin`, le module
JSON dans `%LOCALAPPDATA%\Vitte\modules\json`, et le dossier `bin` est ajouté au
PATH utilisateur. Rouvrez les terminaux après l’installation pour actualiser
leur PATH. Un binaire construit avec le toolchain MSYS2 nécessite son runtime ;
utilisez-le depuis un terminal MSYS2. Pour retirer cette installation, lancez
`make uninstall-windows`.

Sous Linux ou macOS, utilisez `sudo make install` pour installer sous
`/usr/local`, ou définissez `PREFIX` pour choisir un autre préfixe. La cible
`make uninstall` retire les fichiers installés sous ce préfixe.

### Vérifier le seed

Cette cible vérifie le seed directement : version, imports et diagnostics,
puis compilation et exécution de l’exemple `Hello from Vitte!`. Elle ne lance
pas la construction du frontend natif.

```sh
make bootstrap-check
```

Lignes de résultat à repérer, en plus des commandes Make affichées :

```text
vitte <version indiquée dans VERSION>
Vitte compiler
language: Vitte
frontend: C17 bootstrap
Bootstrap targeted import and diagnostic regressions passed.
Hello from Vitte!
Bootstrap seed checks passed.
```

### Stages 0 à 3 du gate self-host

Le cycle part du seed C17, qui compile l’API Vitte pour produire le frontend
natif `build/bin/vitte` (stage 0). Chaque compilateur natif suivant régénère le
C de cette même API ; le compilateur C de l’hôte lie ce C dans un nouvel
exécutable isolé :

```text
sources C17 ──> vitte-bootstrap ──> API C ──> vitte (stage 0)
vitte       ──> stage1-api.c   ──> C17 ──> stage 1
stage 1     ──> stage2-api.c   ──> C17 ──> stage 2
stage 2     ──> stage3-api.c   ──> C17 ──> stage 3
stage 3     ──> Bootstrap Validation ──> Self-Hosting Ready
```

Les noms de fichiers et exécutables permettent d’identifier l’outil qui a
produit chaque artefact :

| Étape / compilateur | C généré | Binaire produit |
| --- | --- | --- |
| Sources C17 du seed | — | `build/bin/vitte-bootstrap` |
| Seed `vitte-bootstrap` | `build/native-frontend/api-implementation.c` | `build/bin/vitte` (stage 0) |
| Stage 0 (`build/bin/vitte`) | `build/selfhost/stage1-api.c` | `build/selfhost/stage1-build/bin/vitte` |
| Stage 1 | `build/selfhost/stage2-api.c` | `build/selfhost/stage2-build/bin/vitte` |
| Stage 2 | `build/selfhost/stage3-api.c` | `build/selfhost/stage3-build/bin/vitte` |

Une première construction complète peut se lancer directement avec :

```sh
make MODE=release bootstrap-selfhost
```

Cette cible construit automatiquement le seed et le frontend natif si besoin,
puis exécute les stages 0 à 3. Pour suivre ou reprendre le cycle par étape,
utilisez les cibles suivantes avec le même `MODE` :

```sh
make MODE=release bootstrap
make MODE=release
make MODE=release bootstrap-stage1
make MODE=release bootstrap-stage2
make MODE=release bootstrap-stage3
```

Le stage 0 vérifie le seed et le frontend natif avec les tests de grammaire,
le sous-ensemble du frontend et la parité seed/native. Chaque stage natif
repasse ces vérifications avant d’être marqué comme validé. Les comparaisons
`cmp` exigent que `stage1-api.c` et `stage2-api.c`, puis `stage2-api.c` et
`stage3-api.c`, soient identiques octet par octet. La validation finale
confirme ainsi que le compilateur reconstruit fonctionne jusqu’au stage 3. Les
messages de réussite attendus incluent :

```text
[SELFHOST] stage0 and stage1 API output match
[SELFHOST] stage1 and stage2 API output match
Native compiler self-host gate passed through stage3.
```

### Critère de fin : compilateur autonome écrit en Vitte

Le gate stage 0–3 prouve que le frontend Vitte se compile de façon stable ; à
lui seul, il ne prouve pas que le binaire principal a cessé d’utiliser le
compilateur C historique. La migration sera terminée quand toutes les
conditions suivantes seront réunies :

1. `build/bin/vitte` entre dans le dispatcher Vitte et retourne son résultat
   sans appeler `vitte_main_entry` ni aucun autre pilote CLI de secours.
2. Toutes les commandes promises par la CLI sont dispatchées et exécutées par
   des modules Vitte. Une commande non disponible doit produire un diagnostic
   Vitte explicite, jamais basculer vers le compilateur historique.
3. Le lien du binaire principal ne contient aucun objet qui implémente le
   lexer, le parseur, l’analyse sémantique, les imports, les diagnostics, le
   backend ou le pilote du compilateur sous `src/bootstrap/`.
4. Le C restant est limité à une liste documentée d’adaptateurs système
   minimaux : entrée du processus, allocation, fichiers et chemins, sortie,
   environnement et lancement des outils hôtes. Ces adaptateurs ne prennent
   aucune décision de compilation.
5. Un audit du lien confirme l’absence des symboles du pilote C historique ;
   la parité CLI, les corpus du langage et les stages de self-host passent sur
   Linux, macOS et Windows.

Le bootstrap peut servir de compilateur seed pendant la construction et rester
archivé pour cette étape. Il ne peut ni participer au lien de `vitte`, ni
prendre en charge une commande à l’exécution. Le travail en cours a déjà
raccordé l’entrée du processus au dispatcher Vitte et limité les objets
sélectionnés par le lien principal aux adaptateurs de frontière. La compilation
release du frontend natif et la génération C17 de l’API passent désormais.
`vitte install` est maintenant dispatché vers un module Vitte avec vérification
SHA-256 et mise à jour transactionnelle des fichiers du projet. L’audit détaillé
des symboles et les gates de parité/self-host sur Linux, macOS et Windows restent
à confirmer.

Les stages vérifiés sont mémorisés sous `build/selfhost/`. Pour reprendre après
un échec, relancez directement la cible du stage concerné :

```sh
make MODE=release bootstrap-stage1
make MODE=release bootstrap-stage2
make MODE=release bootstrap-stage3
```

Chaque cible construit et vérifie les prérequis manquants, puis le stage visé.
Les stages déjà réussis et à jour sont réutilisés. Une erreur au stage 2 se
reprend donc avec `make MODE=release bootstrap-stage2`; le stage 1 ne repasse
ses vérifications que si ses entrées ont changé ou si sa marque de réussite
manque. Le gate complet `bootstrap-selfhost` utilise les mêmes cibles internes.
En cas d’échec, le diagnostic indique le stage et l’opération, les compilateurs
impliqués, les chemins C générés et la cible Make à relancer. La commande
échouée est aussi affichée sous forme de ses arguments.

Chaque opération conserve ses preuves sous
`build/selfhost/evidence/<date>-<commit>-<pid>/stageN/`. Le fichier
`stage-N-summary.txt` indexe les opérations ; chaque fichier `.txt` donne le
résultat, la version Vitte, le commit Git, l’état modifié ou propre du dépôt,
les options de compilation, les compilateurs, les fichiers C et la commande de
reprise. Le `.log` correspondant conserve la sortie complète, notamment les
résultats des corpus de grammaire, du frontend et de parité. Les dossiers des
stages déjà validés restent attachés à leur exécution d’origine quand une
cible reprend plus tard.

### Diagnostiquer une divergence entre sorties C

Les deux comparaisons portent sur ces paires de fichiers :

| Étape de comparaison dans le diagnostic | Sortie du compilateur précédent | Sortie du suivant | Cible à relancer |
| --- | --- | --- | --- |
| `compare generated stage1 and stage2 API C` | `build/selfhost/stage1-api.c` | `build/selfhost/stage2-api.c` | `make MODE=release bootstrap-stage2` |
| `compare generated stage2 and stage3 API C` | `build/selfhost/stage2-api.c` | `build/selfhost/stage3-api.c` | `make MODE=release bootstrap-stage3` |

Exécutez `diff -u` sur la paire indiquée pour repérer la première différence :

```sh
diff -u build/selfhost/stage1-api.c build/selfhost/stage2-api.c
diff -u build/selfhost/stage2-api.c build/selfhost/stage3-api.c
```

Exécutez seulement la commande correspondant à la comparaison en échec.
`diff -u` renvoie le code 1 quand il trouve des différences ; c’est le résultat
attendu pour ce diagnostic. Une sortie vide avec code 0 signifie que les
fichiers sont identiques. Le diagnostic Make donne aussi le manifeste et le
journal de l’étape, sous `build/selfhost/evidence/.../stage2/` ou `stage3/`.
Le manifeste précise le compilateur, le commit, les options et la commande de
reprise ; le journal conserve les sorties standard et d’erreur des commandes,
dont les vérifications du stage ; le manifeste indique le statut exact de la
comparaison `cmp -s`.

Après correction, relancez la cible indiquée avec le même `MODE` que le cycle
initial. L’échec ne rafraîchit pas la marque de réussite du stage ; une marque
plus ancienne reste périmée et Make reprend ce stage. Les stages précédents
validés sont conservés et réutilisés. Pour repasser ensuite toute la chaîne,
lancez `make MODE=release bootstrap-selfhost` : Make ne reconstruit que les
sorties manquantes ou périmées.

Ne lancez pas `make clean`, `make distclean` et ne supprimez pas `build/` ou
`target/` pour diagnostiquer cette divergence. Ces commandes enlèveraient des
artefacts sans aider à localiser la différence ; le parcours de reprise garde
les autres compilateurs, sources C, marques de validation et journaux.

### Gate CI avant publication

La CI appelle le workflow self-host réutilisable sur Linux, macOS et Windows.
Dans chaque lane, `make MODE=release bootstrap-check` valide d’abord le seed C17 ;
le workflow exécute ensuite les tests de parité et le gate complet des stages 0
à 3. Les paquets natifs attendent la réussite du self-host, et le job agrégé
`Release readiness` échoue si les tests compilateur, self-host, paquets ou
portabilité POSIX échouent ou sont ignorés. Les preuves self-host sont publiées
comme artefacts Actions par système.

#### Machines virtuelles BSD, Solaris et illumos

Le workflow réutilisable
[`posix-vm-gates.yml`](.github/workflows/posix-vm-gates.yml) lance six VMs
QEMU via VMActions depuis des runners GitHub Actions Ubuntu hébergés : FreeBSD,
OpenBSD, NetBSD, Solaris, OmniOS et OpenIndiana. Aucun runner personnel n’est
requis. Dans chaque invité, le gate construit le compilateur en release, vérifie
`--version`, `help` et `check`, exécute `vitte run examples/hello.vit`, puis
installe le paquet JSON avec `vitte install`. Le job POSIX est requis par
`Release readiness`.

#### Smoke tests d’architectures 32 bits et embarquées

Le workflow réutilisable
[`architecture-smoke.yml`](.github/workflows/architecture-smoke.yml) est lui
aussi requis par `Release readiness`. Il construit le frontend natif avec un
compilateur hôte, puis vérifie les exécutables croisés lorsqu’un runner peut les
exécuter. Pour FreeBSD, dont les VMs hébergées ne proposent pas d’invité i386 ou
ARMv7, il compile les objets C17 de tout le compilateur dans une VM x86_64 ou
AArch64. Pour Android ARMv7, il lie le compilateur avec le NDK ; l’exécutable
Android n’est pas lancé dans ce gate.

| Cible demandée | Niveau indiqué | Vérification CI |
| --- | --- | --- |
| i386/i686 Linux | Très bonne | Build complet du compilateur, CLI et programme C généré, exécutés sous QEMU i386. |
| i386/i686 FreeBSD | Très bonne | Compilation des objets du compilateur en ABI 32 bits dans une VM FreeBSD x86_64 ; pas d’exécution du binaire i386. |
| i386/i686 Windows | Très bonne | Build et exécution du compilateur, de la CLI et du programme C généré avec MinGW 32 bits sur Windows x64. |
| ARMv6 Linux / Raspberry Pi 1 | Bonne | Build complet et smoke CLI sous QEMU ARMv6, puis exécution du programme C généré avec ABI soft-float. Cela vérifie l’ABI Linux ARMv6, pas un appareil Pi 1 physique. |
| ARMv7 Linux | Bonne | Build complet, CLI et programme C généré exécutés sous QEMU ARMv7 hard-float. |
| ARMv7 FreeBSD | Bonne | Compilation des objets du compilateur avec la cible FreeBSD ARMv7 depuis une VM FreeBSD AArch64 ; pas d’exécution ARMv7. |
| ARMv7 Android | Bonne | Lien complet avec le NDK ARMv7, cible Android API 28, et vérification ELF ; l’exécutable n’est pas lancé sur un appareil ou émulateur Android. |
| RISC-V RV32 Linux embarqué / bare-metal | Expérimentale | Compilation du C d’un petit programme Vitte vers un objet ELF32 RISC-V avec GCC bare-metal (`rv32imac`, ABI `ilp32`) ; pas de lien ni d’exécution du compilateur RV32. |
| MIPS32 Linux | Bonne | Build complet, CLI et programme C généré exécutés sous QEMU MIPS32 little-endian. |
| PowerPC 32 Linux | Possible | Build complet, CLI et programme C généré exécutés sous QEMU PowerPC big-endian ; cela ne vérifie pas Mac OS X PowerPC. |

Ces niveaux décrivent le niveau de support attendu ; les résultats CI ne
certifient que les opérations et environnements indiqués dans la dernière
colonne. Le premier run de ce gate a réussi ; son résultat figure avec les
autres gates dans le tableau ci-dessous.

#### Résultats observés au 7 novembre 2026

Le [run CI 42331580859](https://github.com/vitte-lang/vitte/actions/runs/42331580859), sur le commit [`f460997`](https://github.com/vitte-lang/vitte/commit/f4609970165678dd65048ee4af268e308c5dabcd), a terminé avec succès :

| Gate | Résultat observé |
| --- | --- |
| Tests compilateur et runtime | Réussi : `make test` (16 tests de charge inclus), `make test-runtime`, critères de release, invalidation incrémentale et inventaire des paquets. |
| Self-host, parité native, corpus et stages 0–3 | Réussi sous Linux, macOS et Windows (3/3). |
| Construction et installation des paquets natifs | Réussi sous Linux (`.deb`), macOS (`.pkg`/`.dmg`) et Windows (NSIS), avec vérification des manifestes, installation réelle et smoke `vitte install` (3/3). |
| Portabilité POSIX en VMs | Réussie sous FreeBSD, OpenBSD, NetBSD, Solaris, OmniOS et OpenIndiana (6/6). |
| Architectures 32 bits et embarquées | Réussi : 10/10 jobs, avec Linux i386, ARMv6, ARMv7, MIPS32 et PowerPC32, Windows i386, FreeBSD i386/ARMv7, Android ARMv7 et l’objet bare-metal RV32. |
| Extension et DAP natif | Réussi. |
| Jobs de ce run | **100 % : 25/25 réussis**, sans job ignoré. |
| Gate agrégé `Release readiness` de ce run | **Réussi.** |

Le job compilateur a exécuté les 16 tests de charge en 371,3 s sur le runner
Linux, avec une entrée de 2 Mio en profil debug et un délai maximal de 180 s
par processus. Le profil release conserve les entrées de 48 Mio. Sur le poste
macOS local, `make MODE=release test`, `make MODE=release test-runtime`,
`make MODE=release test-native-bootstrap-parity` et
`make MODE=release bootstrap-selfhost` ont également réussi ; le corpus a
vérifié 93 cas positifs, 18 cas négatifs, zéro écart connu et zéro échec
(2 cas ignorés). Les stages self-host 0–3 ainsi que les commandes CLI `help`,
`version`, `explain` et `install`, y compris le rollback d’installation,
ont été vérifiés.

Le statut **100 %** ci-dessus décrit précisément le run cité : 25 jobs réussis
sur 25, couvrant compilateur/runtime, self-host (3 systèmes), installation de
paquets (3 systèmes), portabilité VM (6 systèmes), architectures (10 jobs),
extension/DAP et le gate agrégé `Release readiness`. Le statut s’applique au
commit `f460997` et à ce run CI ; il ne constitue pas à lui seul une déclaration
d’autonomie complète du compilateur.

Le dépôt contient une seule unité C++ :
`editors/tree-sitter/bindings/node/binding.cc`, liée à la liaison Node de
Tree-sitter pour l’éditeur, hors du binaire du compilateur. Les sources C
restantes du lien natif assurent le seed bootstrap, le système de fichiers
et les frontières OS/processus/stdio.

L’audit des symboles du binaire principal a identifié l’objet filesystem C de
`src/bootstrap` comme dépendance résiduelle. La politique de création récursive
des répertoires utilisée par le frontend natif est implémentée dans
`src/compiler/filesystem/filesystem.vitte`. L’entrée C historique reste disponible
pour le bootstrap et la compatibilité ABI ; la frontière native conserve l’appel
système de création d’un répertoire et l’information sur le séparateur de
plateforme. Le frontend utilise toujours sept opérations filesystem C pour les
métadonnées, le répertoire courant, le listing,
la suppression, le renommage et la libération des listes. Le module exporte
27 symboles : 20 autres ne sont pas référencés par le frontend, mais restent
inclus parce que l’objet est lié directement. Le gate
`compiler-binary-ownership` vérifie cet inventaire et les imports actifs, y
compris les deux dépendances de frontière filesystem. Le portage des opérations
filesystem restantes et le retrait de l’objet C sont nécessaires avant de
qualifier le binaire d’autonome.

La branche `main` exige le statut `Release readiness`, avec les vérifications à
jour et sans contournement administrateur. Les règles de protection GitHub sont
une configuration du dépôt distincte des fichiers du workflow ; publiez une
version depuis `main` seulement après la réussite de ce statut.

Pour la comparaison rapide du parseur, de l’AST et de l’analyse sémantique entre
le seed et le frontend natif, utilisez :

```sh
make test-native-bootstrap-parity
```

Sorties attendues pour les cas actuellement suivis :

```text
Native/bootstrap parser and AST parity passed: 7 cases.
Native/bootstrap semantic parity passed: 4 cases.
```

La suite de régression complète se lance avec `make test`. CMake est également pris en
charge avec `cmake -S . -B build && cmake --build build`.

La couverture actuelle du langage et ses limites connues sont répertoriées dans
`src/docs/language_coverage.md`.
