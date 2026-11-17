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
| **Validation multiplateforme** | Les tests couvrent le compilateur, le runtime, les paquets et la portabilité. |

## Démarrage rapide

**Prérequis :** GCC ou Clang compatible C17, Steel et Python 3 pour les vérifications.

Depuis la racine du dépôt :

```sh
# Construire Vitte
steel run --file steelconf

# Vérifier la version
./build/bin/vitte --version

# Exécuter un premier programme
./build/bin/vitte run examples/hello.vit
```

> **Steelconf est le point d’entrée recommandé** pour les nouvelles intégrations. Le Makefile reste disponible pendant la migration.

<details>
<summary><strong>Vous préférez Make ?</strong></summary>

```sh
# Construire le frontend natif
make

# Choisir le compilateur C
make CC=clang

# Construire uniquement le seed C17
make bootstrap
./build/bin/vitte-bootstrap --version
```

Sous Windows avec MSYS, le seed est produit sous le nom `vitte-bootstrap.exe`.

</details>

## Les commandes à retenir

| Action | Commande |
| :--- | :--- |
| Construire | `steel run --file steelconf` |
| Construire avec 4 jobs | `steel run --file steelconf --jobs 4` |
| Vérifier | `steel run --file steelconf --bake check` |
| Tester | `steel run --file steelconf --bake test` |
| Nettoyer | `steel clean --file steelconf` |
| Installer | `steel install --file steelconf` |

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

Selon la cible, les vérifications portent sur l’exécution, la compilation d’objets ou le lien. Elles ne représentent pas toutes le même niveau de support.

[Consulter la matrice de support →](https://github.com/vitte-lang/vitte/blob/main/docs/reference/support.html)

## Explorer Vitte

| Vous souhaitez… | Ressource |
| :--- | :--- |
| Parcourir la documentation | [Documentation](https://github.com/vitte-lang/vitte/tree/main/docs) |
| Connaître les limites actuelles | [Couverture du langage](https://github.com/vitte-lang/vitte/blob/main/src/docs/language_coverage.md) |
| Comprendre le build | [Configuration Steelconf](https://github.com/vitte-lang/vitte/blob/main/steelconf) |
| Suivre les validations | [GitHub Actions](https://github.com/vitte-lang/vitte/actions) |
| Signaler un problème | [Issues](https://github.com/vitte-lang/vitte/issues) |

---

<div align="center">

**Explorez le code. Construisez Vitte. Essayez le langage.**

[Découvrir le projet →](https://vitte-lang.org/)

</div>
