
# compiler/third_party — dépendances vendorisées (max)

Ce dossier est réservé aux **dépendances tierces vendorisées** utilisées par le compilateur (`compiler/`).

Principe :
- **Optionnel** : préférer les deps via toolchain/package manager quand c’est raisonnable.
- **Vendoring** uniquement si nécessaire : reproductibilité, patchs, offline builds, contraintes CI.

Ce dossier n’est **pas** un dump. Il est soumis à des règles strictes (licences, traçabilité, intégrité).

---

## 1) Quand vendoriser ?

Vendoriser seulement si au moins un des points suivants est vrai :

- dépendance critique pour le build et non disponible partout
- besoin de **patch** local
- besoin de **version figée** (reproductible)
- environnement CI/offline
- dépendance trop petite pour justifier un gestionnaire externe

Sinon :
- utiliser la toolchain système / package manager / submodule (selon policy projet)

---

## 2) Règles de structure

Chaque dépendance vit dans un dossier dédié :

```
compiler/third_party/
  README.md
  <dep_name>/
    UPSTREAM.md
    LICENSE
    NOTICE            (optionnel)
    PATCHES/          (optionnel)
    SRC/              (sources vendorisées)
    SHA256SUMS        (optionnel)
```

### 2.1 `<dep_name>`

- nom stable, lowercase, sans espaces
- exemple : `xxhash/`, `utf8proc/`, `pcre2/`

### 2.2 UPSTREAM.md (obligatoire)

Contenu minimal :
- nom du projet
- version / tag / commit
- source (repo, archive)
- date d’import
- modifications locales (liste)
- commande/repro pour réimporter

### 2.3 LICENSE / NOTICE

- `LICENSE` obligatoire (copie exacte)
- `NOTICE` si le projet l’exige (Apache-2.0, etc.)

---

## 3) Politiques de licence

### 3.1 Licences acceptables (recommandation)

- MIT
- BSD-2/BSD-3
- Apache-2.0
- ISC
- Zlib

### 3.2 Licences à éviter / interdites (par défaut)

- GPL/AGPL (sauf décision explicite du projet)
- licences ambiguës / non redistribuables

Règle :
- si une dépendance a une licence non standard, documenter explicitement la décision (UPSTREAM.md + docs).

---

## 4) Intégrité et traçabilité

### 4.1 Checksums

Recommandé :
- conserver un `SHA256SUMS` pour l’archive upstream ou la tarball importée.

### 4.2 Patchs

- Toute modification locale doit être dans `PATCHES/` si possible.
- Préférer une série de patchs plutôt que de modifier directement `SRC/`.
- Si modification directe : documenter précisément dans UPSTREAM.md.

### 4.3 Minimiser le contenu

- supprimer tests/examples/CI upstream si inutiles au build
- ne pas vendoriser des binaires
- ne pas vendoriser des fichiers générés volumineux

---

## 5) Build integration

Règles :
- Les sources vendorisées doivent être intégrées via le système de build du compilateur.
- Ne pas ajouter d’étapes non déterministes.
- Les flags doivent respecter :
  - `spec/targets/flags.md`
  - standard C du projet (C23)

Recommandation :
- isoler les flags spécifiques de la dep dans un module build dédié.

---

## 6) Mise à jour d’une dépendance

Procédure recommandée :

1) Identifier upstream : tag/commit
2) Importer proprement dans `SRC/`
3) Mettre à jour `UPSTREAM.md`
4) Appliquer/rebaser `PATCHES/`
5) Exécuter :
   - build debug + release
   - tests concernés
   - fuzz short-run si la dep touche parse/format
6) Mettre à jour `SHA256SUMS` (si utilisé)

---

## 7) Sécurité

- Éviter les dépendances “supply chain” instables.
- Préférer des versions taggées.
- Documenter les CVEs connues si pertinentes.

Optionnel (future) :
- SBOM / inventory des deps vendorisées.

---

## 8) Exemple de UPSTREAM.md

```
# utf8proc (vendored)

Upstream:
- Project: utf8proc
- Source: https://github.com/JuliaStrings/utf8proc
- Version: v2.9.0 (tag)
- Commit: <sha>
- Imported: 2025-12-16

Local changes:
- PATCHES/0001-disable-benchmark-build.patch

Reimport:
- Download release tarball
- Verify sha256 in SHA256SUMS
- Replace SRC/
- Reapply PATCHES/
```

---

## 9) Notes

- Toute nouvelle dépendance doit être justifiée (PR description).
- Si une dépendance peut être remplacée par une implémentation stdlib/locale, préférer la suppression.

