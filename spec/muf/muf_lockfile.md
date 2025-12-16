# MUF Lockfile (muffin.lock) — format & règles de résolution

Ce document spécifie le **format** et les **règles de résolution** du lockfile MUF.

Objectifs :
- Rendre les builds **reproductibles** (versions, sources, révisions, checksums).
- Rendre la résolution **déterministe** (mêmes entrées, même ordre, mêmes hashes).
- Être **diff-friendly** et stable dans le temps.

Le lockfile est un fichier **généré** (ne pas éditer à la main, sauf debug).

---

## 1) Fichiers concernés

- **Manifest** : `muffin.muf` (source de vérité fonctionnelle)
- **Lockfile** : `muffin.lock` (état résolu et figé)

Le lockfile utilise la **même syntaxe MUF** (voir `muf_grammar.ebnf`) mais avec un sous-ensemble strict.

---

## 2) Structure générale du lockfile

Un lockfile contient :
- des **métadonnées** (format, version, générateur, date)
- un ensemble d'**entrées** (`entry`) représentant chaque dépendance résolue

### 2.1 En-tête minimal

Champs recommandés :
- `lock.format` : ident de format (ex: `"muf-lock"`)
- `lock.version` : version du lockfile (ex: `v1.0.0`)
- `lock.generator` : outil et version (ex: `"muffin v0.1.0"`)
- `lock.generated_at` : ISO-8601 (ex: `"2025-12-15T23:00:00+01:00"`)

### 2.2 Entrées

Chaque dépendance résolue est une entrée :

```muf
lock
  lock.format = "muf-lock"
  lock.version = v1.0.0
  lock.generator = "muffin v0.1.0"
  lock.generated_at = "2025-12-15T23:00:00+01:00"

  entry foo
    name = "foo"
    version = v1.2.3
    source.kind = "registry"
    source.url = "https://registry.vitte.dev"
    checksum.sha256 = "..."
    deps = ["bar@v2.0.1", "baz@v0.9.0"]
    features = ["serde", "fast"]
  .end
.end
```

Notes :
- `entry <Ident>` est un ident court (slug). `name` peut être plus explicite.
- Les clés utilisent le style `a.b.c` (KvStmt standard).

---

## 3) Modèle de données d'une entrée

### 3.1 Champs obligatoires

- `name` : string
- `version` : token `Version` (voir grammaire)
- `source.kind` : string ∈ {`"path"`, `"git"`, `"registry"`}

### 3.2 Champs source

#### a) Source `path`
- `source.path` : path ou string
- `source.canonical` : string (chemin canonique normalisé, optionnel)

#### b) Source `git`
- `source.url` : string (URL Git)
- `source.rev` : string (commit SHA, tag annoté résolu en SHA, ou ref figée)
- `source.subdir` : string (optionnel)

#### c) Source `registry`
- `source.url` : string (URL registry)
- `source.package` : string (nom côté registry, si différent)

### 3.3 Checksums

- `checksum.sha256` : string hex

Règle : le checksum doit porter sur un **artefact source canonique** (voir §7).

### 3.4 Dépendances transitive

- `deps` : array de strings au format canonique :
  - `"<name>@<version>"`

Optionnel :
- `deps.optional` : array (dépendances optionnelles)
- `deps.dev` : array (dépendances de dev/tests/outils)

### 3.5 Features

- `features` : array de strings (features activées pour **cette** entrée)

Optionnel :
- `features.default` : bool (si le resolver applique les defaults)

### 3.6 Plateformes / targets

Optionnel (si le resolver fait du lock multi-target) :
- `targets` : array de strings (ex: `"x86_64-unknown-linux-gnu"`)

Règle : si `targets` est absent, l'entrée est considérée **agnostique**.

---

## 4) Canonicalisation & ordre déterministe

### 4.1 Normalisation des clés

- Clés en `lower_snake_case` recommandées.
- Les espaces autour de `=` sont libres mais la sortie générée doit respecter un style unique.

### 4.2 Ordre

Le lockfile généré doit être stable :
- Les `entry` sont triées par `(name, version, source.kind, source.url|source.path, source.rev)`
- Les arrays (`deps`, `features`, `targets`) sont triés lexicographiquement

### 4.3 Identité d'une entrée (EntryId)

Identité logique :

```
EntryId = (name, version, source.kind, source locator, source.rev?)
```

- `source locator` = `path` (canon) ou `url` (git/registry)

---

## 5) Règles de résolution (resolver)

### 5.1 Entrées seed

Le resolver part de :
- `workspace` + modules locaux
- la section `deps` (globale et/ou dans `module ... deps ... .end`)
- les features sélectionnées (CLI ou manifest)

### 5.2 Résolution des versions

Politique par défaut :
- Si une dépendance a une version **exacte** (ex: `v1.2.3`), elle est figée.
- Si une dépendance a une contrainte (future extension : `^1.2`, `~1.2`, `>=1.2 <2.0`), le resolver choisit la **plus haute** version satisfaisante.

Si la grammaire MUF ne porte pas (encore) les contraintes, elles doivent être **désucrées** en version exacte au moment du lock.

### 5.3 Unification vs multi-version

- **Mode unification (par défaut)** :
  - un même `name` doit résoudre vers une seule version compatible
  - en cas de conflit : erreur `E_LOCK_CONFLICT`

- **Mode multi-version (optionnel)** :
  - autorise plusieurs versions du même `name`
  - chaque entrée garde ses `deps` résolues vers des `name@version`

Le mode est contrôlé par un champ (si présent) :
- `lock.mode = "unify" | "multi"`

### 5.4 Résolution des sources

Priorité de résolution (par défaut) :
1) `path` (si explicitement demandé)
2) `git`
3) `registry`

Une dépendance ne doit pas changer de `source.kind` lors d'un `muffin build` normal.
Le changement de source doit être une action explicite (`muffin update`, `muffin vendor`, etc.).

### 5.5 Déduplication

Deux dépendances sont dédupliquées si et seulement si leurs `EntryId` sont identiques.

---

## 6) Politique de mise à jour

Commandes attendues (comportement) :
- `muffin lock` : (ré)génère `muffin.lock` à partir des manifests, sans upgrade implicite
- `muffin update <dep?>` : autorise upgrade selon contraintes, puis regénère
- `muffin vendor` : matérialise les sources (cache/`vendor/`) et met à jour `source.path` si nécessaire

---

## 7) Artefact canonique & checksum

### 7.1 Définition

Le checksum `checksum.sha256` porte sur un **tarball source canonique** (ou équivalent) construit de façon déterministe :
- Chemins normalisés (`/`)
- Pas de timestamps dans l'archive (ou timestamp fixé à 0)
- Tri lexicographique des fichiers
- Permissions normalisées
- Exclusion des artefacts de build (`/target`, `/build`, `.git/`, etc.)

### 7.2 Git

Pour `git`, l'artefact canonique correspond au contenu du tree au `rev` + `subdir` éventuel.
Le `rev` doit être un SHA immuable.

### 7.3 Registry

Pour `registry`, l'artefact canonique est celui téléchargé (source archive) et validé par hash.

---

## 8) Erreurs de résolution

Erreurs standard :
- `E_LOCK_CONFLICT` : contraintes incompatibles en mode unification
- `E_LOCK_MISSING` : lockfile absent alors que politique exige lock strict
- `E_SOURCE_UNPINNED` : source git sans `rev` (non permis en lock)
- `E_CHECKSUM_MISMATCH` : artefact téléchargé != checksum
- `E_CYCLE` : cycle de dépendances (si interdit par la sémantique)

---

## 9) Exemple complet

```muf
lock
  lock.format = "muf-lock"
  lock.version = v1.0.0
  lock.mode = "unify"
  lock.generator = "muffin v0.1.0"
  lock.generated_at = "2025-12-15T23:00:00+01:00"

  entry fmt
    name = "fmt"
    version = v2.1.0
    source.kind = "registry"
    source.url = "https://registry.vitte.dev"
    checksum.sha256 = "8f2d..."
    deps = ["unicode@v1.0.0"]
    features = ["fast"]
  .end

  entry unicode
    name = "unicode"
    version = v1.0.0
    source.kind = "git"
    source.url = "https://github.com/roussov/vitte-unicode"
    source.rev = "2d9a4c3d3c2e1c0f9e0a8b7c6d5e4f3a2b1c0d9e"
    checksum.sha256 = "1a7c..."
    deps = []
    features = []
  .end
.end
```

---

## 10) Compatibilité & évolution

- Toute évolution du lockfile doit être **versionnée** via `lock.version`.
- Les parseurs doivent refuser silencieusement de « deviner » :
  - si `lock.format` inattendu → erreur
  - si `lock.version` majeure inconnue → erreur
