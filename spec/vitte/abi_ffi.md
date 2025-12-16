# ABI / FFI — modèle stable Vitte (runtime, plugins, PAL)

Ce document décrit le **modèle ABI/FFI** de Vitte : conventions, types stables, symboles, versioning.

Il complète :
- `spec/runtime/rt_abi.md` (ABI runtime : slices/strings/handles/errors + plugins)
- `spec/runtime/pal_contract.md` (contrat PAL)

Objectifs :
- ABI C99 **portable** et **stable** (C, Rust, Zig, Swift, etc.).
- Layouts déterministes : tailles/alignements définis, append-only.
- Ownership explicite : qui alloue, qui libère.
- Chargement plugins sûr : handshake de version + caps.

Non-objectifs :
- Définir l’ABI des types haut niveau `std/*`.
- Spécifier le bytecode ou l’IR.

---

## 1) Surface ABI (ce qui est “public”)

### 1.1 Headers publics (recommandation)

Le runtime expose les headers dans `include/` :
- `include/vitte/rt_abi.h` : types `result`, `slice`, `str`, `handle`, vtables, plugin entry
- `include/vitte/pal.h` : types PAL + vtable PAL (si l’embedder fournit une PAL externe)
- `include/vitte/version.h` : versions (runtime ABI, PAL ABI)

Règle :
- Les headers publics ne doivent inclure que des headers standard C (`stdint.h`, `stddef.h`).

### 1.2 Symboles publics

Catégories :
- **runtime ABI** : `vitte_rt_abi_version`, `vitte_rt_api` etc.
- **plugin ABI** : `vitte_plugin_entry` (export stable)
- **PAL ABI** : `vitte_pal_api` (si fourni/chargé) + `pal_version/capabilities`

---

## 2) Conventions ABI (C)

### 2.1 Langage / linkage

- Toutes les fonctions ABI doivent être compatibles **C**.
- En C++ : entourer les déclarations par `extern "C"`.

### 2.2 Calling convention

- Par défaut : convention C de la plateforme.
- Windows : **cible** unique recommandée : `__cdecl`.

### 2.3 Struct layout

- Interdit : `#pragma pack` sur types ABI.
- Alignements naturels.
- Pas de bitfields dans les structs ABI.

### 2.4 Endianness

- ABI = endianness native.
- Toute sérialisation (fichier/IPC) doit définir explicitement son endianness.

---

## 3) Versioning & compat

### 3.1 SemVer

Les contrats ABI runtime et PAL sont versionnés en SemVer :
- `vMAJOR.MINOR.PATCH`

Règles :
- **MAJOR** : rupture ABI.
- **MINOR** : ajout backward-compatible (append-only).
- **PATCH** : bugfix.

### 3.2 Append-only (règle absolue)

Pour toute struct ABI (vtable, desc/out, headers) :
- jamais réordonner
- jamais supprimer
- ajouter uniquement en fin

### 3.3 Négociation

- Un plugin refuse le chargement si `abi_major` diffère.
- Un embedder (hôte) refuse une PAL si `pal_version.major` diffère.

---

## 4) Types stables (FFI-safe)

### 4.1 Entiers

- utiliser `<stdint.h>`

### 4.2 Bool

```c
typedef uint8_t vitte_bool; /* 0 ou 1 */
```

### 4.3 Handles

```c
typedef uint64_t vitte_handle; /* 0 = invalid */
```

Handles typés par alias (même représentation) :
- `vitte_file_handle`, `vitte_socket_handle`, `vitte_thread_handle`, etc.

Règle :
- une fonction recevant `0` doit renvoyer `VITTE_EBADF`.

### 4.4 Result

```c
typedef uint32_t vitte_errc;

typedef struct {
  vitte_errc code;   /* 0 = OK */
  uint32_t   subcode;/* info OS/impl pour debug */
} vitte_result;
```

Règles :
- `subcode` ne doit pas être utilisé pour la logique métier.
- Les codes sont stables et “mappables” depuis la PAL.

### 4.5 Strings & slices

Slice bytes non possédé :

```c
typedef struct {
  const uint8_t* ptr;
  uint64_t       len;
} vitte_slice_u8;
```

String UTF-8 non possédée :

```c
typedef struct {
  const char* ptr; /* UTF-8, pas forcément NUL-terminated */
  uint64_t    len; /* bytes */
} vitte_str;
```

Règles :
- Le consommateur ne conserve pas les pointeurs après retour.
- Si une API exige NUL-terminated, cela doit être explicitement mentionné.

---

## 5) Ownership, mémoire, allocations

### 5.1 Principe général

- Qui alloue → libère.
- Ne jamais libérer via `free()` un buffer alloué par l’autre côté.

### 5.2 Allocateur runtime (interface)

Le runtime expose (dans sa vtable) :
- `alloc(size, align, *out_ptr)`
- `free(ptr, size, align)`

Règle :
- Le couple `(size, align)` passé à `free` doit être identique.

### 5.3 Pattern “write into caller buffer”

Pour éviter les allocations inter-domaines :
- le caller fournit `(buf, cap)`
- l’API renvoie `needed`

Recommandation :
- retourner `VITTE_EAGAIN` si `cap < needed`.

---

## 6) Visibilité des symboles (dylib)

Macro recommandée :

```c
#if defined(_WIN32)
  #define VITTE_EXPORT __declspec(dllexport)
  #define VITTE_IMPORT __declspec(dllimport)
#else
  #define VITTE_EXPORT __attribute__((visibility("default")))
  #define VITTE_IMPORT
#endif
```

Règles :
- Compiler en `-fvisibility=hidden` et exporter uniquement l’ABI.
- Le nom export plugin est **fixe** : `vitte_plugin_entry`.

---

## 7) ABI plugins (contrat de chargement)

### 7.1 Symbole d’entrée

Chaque plugin (dylib) exporte :

```c
VITTE_EXPORT vitte_result vitte_plugin_entry(
  const struct vitte_rt_api* rt,
  const struct vitte_plugin_desc* desc,
  struct vitte_plugin_out* out
);
```

- `rt` : vtable runtime fournie par l’hôte
- `desc` : infos hôte → plugin
- `out` : infos plugin → hôte

### 7.2 Desc runtime → plugin

```c
typedef struct vitte_plugin_desc {
  uint32_t abi_major, abi_minor, abi_patch;
  uint64_t capabilities;
  vitte_str requested_name; /* optionnel */
} vitte_plugin_desc;
```

### 7.3 Out plugin → runtime

```c
typedef struct vitte_plugin_out {
  uint32_t plugin_abi_major, plugin_abi_minor, plugin_abi_patch;
  uint64_t plugin_caps;
  const void* vtable; /* plugin-defined, append-only */
  vitte_str   name;
  vitte_str   version;
} vitte_plugin_out;
```

Règles :
- `plugin_abi_major` doit matcher `abi_major`.
- Le plugin ne doit pas garder `rt` au-delà de la durée de vie définie par l’hôte (contrat de l’hôte).

---

## 8) Vtables runtime & PAL

### 8.1 Vtable runtime (minimum)

Le runtime expose une vtable append-only :

```c
typedef struct vitte_rt_api {
  uint32_t abi_major, abi_minor, abi_patch;

  vitte_result (*alloc)(uint64_t size, uint64_t align, void** out_ptr);
  vitte_result (*free)(void* ptr, uint64_t size, uint64_t align);

  void (*log)(uint32_t level, vitte_str msg);
  uint32_t (*last_os_error)(void);

  vitte_result (*utf8_validate)(vitte_str s); /* optionnel */

  /* ... append-only ... */
} vitte_rt_api;
```

Règles :
- `log` doit être safe (pas de deadlock, pas d’alloc non contrôlée).
- `utf8_validate` peut renvoyer `VITTE_ENOTSUP`.

### 8.2 Vtable PAL

La PAL est décrite dans `spec/runtime/pal_contract.md`.

Règle d’architecture :
- un plugin **ne doit pas** appeler la PAL directement.
- un plugin passe par le runtime (policy/capabilities/sandbox).

---

## 9) Erreurs & mapping

### 9.1 Table de codes recommandée

Le runtime et la PAL doivent partager une table cohérente :
- `OK, EINVAL, ENOTSUP, ENOMEM, EIO, EACCES, ENOENT, EEXIST, EBUSY, ETIMEDOUT, EINTR, EAGAIN, EPIPE, ECLOSED, EBADF, EOF, EOVERFLOW, ESTATE, EABI`

### 9.2 Compat POSIX / Windows

- POSIX : `subcode` peut contenir `errno`.
- Windows : `subcode` peut contenir `GetLastError()` ou `WSAGetLastError()`.

Règle :
- la logique métier ne doit pas dépendre de `subcode`.

---

## 10) Threads & callbacks

Principe :
- L’ABI doit expliciter la thread-safety.

Règles recommandées :
- Le runtime peut appeler un plugin depuis plusieurs threads.
- Toute callback fournie par un plugin doit documenter sa thread-safety.

Extension future (prévue) :
- callbacks host→plugin standardisées avec timeouts.

---

## 11) Bindings (langages)

### 11.1 Rust

Recommandations :
- `#[repr(C)]` sur toutes les structs.
- `u8` pour bool.
- éviter `bool` Rust dans l’ABI.

### 11.2 Swift / Zig / others

- privilégier wrappers générés.
- ne pas exposer de types dépendants du runtime (ex: `size_t` variable) sans encapsulation.

---

## 12) Checklist conformité ABI

Toute impl runtime/host/plugin doit vérifier :
- `sizeof(vitte_handle) == 8`
- `sizeof(vitte_result) == 8`
- layouts `repr(C)`/C natif
- symbol export : `vitte_plugin_entry`
- append-only respecté

---

## 13) Notes

- Le contrat ABI est strict : une divergence de layout est une faille.
- Toute évolution doit passer par incrément de version et append-only.
