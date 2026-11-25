<div align="center">

# Vitte

### Le compilateur officiel du langage Vitte.

**Un frontend écrit en Vitte. Une sortie C17. Un chemin vers l’auto-compilation.**

[![Vitte CI](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml/badge.svg)](https://github.com/vitte-lang/vitte/actions/workflows/ci.yml)

[Démarrer](#démarrage-rapide) · [Documentation](https://github.com/vitte-lang/vitte/tree/main/docs) · [Code source](https://github.com/vitte-lang/vitte) · [Site officiel](https://vitte-lang.org/)

</div>

---

## Du code Vitte au C17

Vitte transforme votre code source en C17 à travers un frontend écrit en Vitte. Un compilateur initial en C17 amorce sa construction ; des adaptateurs C assurent les échanges avec le système et les outils hôtes.

```text
Source → Lexer → Parser / AST → Analyse sémantique → HIR → IR → C17
```

| Au cœur de Vitte | En pratique |
| :--- | :--- |
| **Frontend en Vitte** | Le langage participe à la construction de son propre compilateur. |
| **Sortie C17** | Le pipeline s’appuie sur les outils C de votre environnement. |
| **Bootstrap progressif** | Plusieurs générations du frontend sont reconstruites et comparées. |
| **Validation multiplateforme** | La CI couvre trois cibles natives candidates et des expériences de portabilité; plusieurs gates sont encore rouges. |

## Démarrage rapide

**Prérequis :** GNU Make et GCC ou Clang compatible C17. Python 3 est requis pour les vérifications. Steel est facultatif et fournit des alias vers les cibles Make.

Depuis la racine du dépôt :

```sh
# Construire le frontend natif et son seed C17
make -j2

# Vérifier la version
vitte --version

# Exécuter un premier programme
vitte run examples/hello.vit
```

Le Makefile définit les sources, options C, dépendances et sorties. `steelconf` délègue les mêmes cibles à Make; il ne maintient pas une seconde recette de compilation.

<details>
<summary><strong>Choisir les options de build</strong></summary>

```sh
# Construire le frontend natif
make

# Choisir le compilateur C
make CC=clang

# Construire uniquement le seed C17
make bootstrap
vitte-bootstrap --version
```

Sous Windows avec MSYS, le seed est produit sous le nom `vitte-bootstrap.exe`.

</details>

## Les commandes à retenir

| Action | Make | Alias Steel facultatif |
| :--- | :--- | :--- |
| Construire | `make -j2` | `steel run --file steelconf` |
| Construire le seed C17 | `make bootstrap` | `steel run --file steelconf --bake bootstrap` |
| Vérifier les sources C | `make check` | `steel run --file steelconf --bake check` |
| Vérifier le seed | `make bootstrap-check` | `steel run --file steelconf --bake bootstrap_check` |
| Suite complète | `make test` | `steel run --file steelconf --bake test` |
| Bootstrap stages 0→3 | `make MODE=release bootstrap-selfhost` | `steel run --file steelconf --bake bootstrap_selfhost` |
| Nettoyer | `make clean` | — |
| Installer | `make install` | — |

Les noms de bakes Steel qui correspondent à des cibles Make avec tirets utilisent des soulignés (`bootstrap_check`, `bootstrap_selfhost`). Steelconf reprend les variables d’environnement telles que `CC` et `PREFIX`; le nombre de jobs se règle avec `make -jN`.

## Installer Vitte

### Linux et macOS

Pour installer sous `/usr/local` avec Make :

```sh
sudo make install
```

Vous pouvez choisir un autre emplacement avec `PREFIX`.

### Windows

Depuis le terminal **MSYS2** du dépôt :

```sh
make install-windows
```

Le compilateur est installé dans `%LOCALAPPDATA%\Programs\Vitte\bin`, puis ajouté au PATH utilisateur. Rouvrez votre terminal après l’installation.

Un binaire construit avec MSYS2 nécessite son runtime : utilisez-le depuis un terminal MSYS2.

<details>
<summary>Désinstallation</summary>

**Linux et macOS :**

```sh
make uninstall
```

Utilisez le même préfixe et les droits nécessaires à l’installation.

**Windows :**

```sh
make uninstall-windows
```

</details>

## Un compilateur qui se reconstruit

Le cycle d’auto-compilation reconstruit l’API du frontend jusqu’au **stage 3**, puis compare les sorties C successives.

```text
Seed C17 → Frontend Vitte → Stage 1 → Stage 2 → Stage 3
```

Pour lancer le cycle complet :

```sh
make MODE=release bootstrap-selfhost
```

Pour comparer rapidement le seed et le frontend natif :

```sh
make test-native-bootstrap-parity
```

> **État du projet**
>
> L’auto-compilation stable et l’autonomie complète sont deux jalons distincts. Certaines opérations de système de fichiers reposent encore sur du C. Leur portage et le retrait de l’objet C résiduel restent nécessaires avant de qualifier le binaire d’autonome.

## La qualité fait partie du build

Les validations couvrent :

- **Le compilateur et le runtime** : régressions, corpus du langage et tests de charge.
- **L’auto-compilation** : parité et stages successifs sur Linux, macOS et Windows.
- **Les paquets natifs** : construction, installation et vérifications après installation.
- **La portabilité POSIX** : FreeBSD, OpenBSD, NetBSD, Solaris, OmniOS et OpenIndiana.
- **Les architectures ciblées** : x86 32 bits, ARM, MIPS, PowerPC et RISC-V.

Ces jobs expérimentaux ne sont pas des plateformes officiellement supportées. Les seules cibles natives envisagées pour 0.1.1 sont Linux x86_64, macOS arm64 et Windows x86_64/MSYS2 UCRT64; les trois gates ont encore des échecs et la publication est différée. Les versions exactes des toolchains C ne sont pas épinglées. La matrice de support distingue les cas vérifiés du reste du langage.

[Consulter la matrice de support →](https://github.com/vitte-lang/vitte/blob/main/docs/reference/support.html)

[Consulter l’audit 0.1.1, les reproductions et les reports →](https://github.com/vitte-lang/vitte/blob/main/src/docs/compiler_0.1.1_audit.md)

## Explorer Vitte

| Vous souhaitez… | Ressource |
| :--- | :--- |
| Parcourir la documentation | [Documentation](https://github.com/vitte-lang/vitte/tree/main/docs) |
| Connaître les limites actuelles | [Couverture du langage](https://github.com/vitte-lang/vitte/blob/main/src/docs/language_coverage.md) |
| Suivre la stabilisation 0.1.1 | [Audit, CI et décisions de release](https://github.com/vitte-lang/vitte/blob/main/src/docs/compiler_0.1.1_audit.md) |
| Comprendre le build | [Configuration Steelconf](https://github.com/vitte-lang/vitte/blob/main/steelconf) |
| Suivre les validations | [GitHub Actions](https://github.com/vitte-lang/vitte/actions) |
| Signaler un problème | [Issues](https://github.com/vitte-lang/vitte/issues) |

---

<div align="center">

**Explorez le code. Construisez Vitte. Essayez le langage.**

[Découvrir le projet →](https://vitte-lang.org/)

</div>
