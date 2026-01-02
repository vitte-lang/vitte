# TODO: runtime ABI (slices/handles)

# Runtime ABI — slices, strings, handles, erreurs (contrat stable)

Ce document définit l’ABI **stable** entre :
- le **runtime/VM** (implémenté en C),
- les **modules natifs** (plugins/dylibs),
- et la couche **PAL** (voir `pal_contract.md`).

Objectifs :
- ABI C **simple** (C99) et **portable**.
- Types à tailles fixes, alignements stables.
- Ownership explicite (qui alloue / qui libère).
- Zéro exception : tout passe par `Result`.

Non-objectifs :
- Définir l’ABI “haut niveau” de `std/*`.
- Spécifier le bytecode (voir specs VM).

---

## 1) Versioning & compatibilité

### 1.1 Version du runtime ABI

- L’ABI runtime est versionnée en SemVer : `vMAJOR.MINOR.PATCH`.
- **MAJOR** : rupture ABI (layout/type, signature, calling conv).
- **MINOR** : ajout backward-compatible (nouvelles fonctions, nouveaux champs en fin de struct).
- **PATCH** : bugfix sans changement de contrat.

Le runtime expose :
- `vitte_rt_abi_version()` → `{major, minor, patch}`

Un plugin doit :
- refuser de se charger si `major` ne matche pas.

### 1.2 Append-only

Toute struct publique (vtable, header) est **append-only** :
- jamais réordonner
- jamais supprimer
- uniquement ajouter des champs en fin

---

## 2) Conventions ABI (C)

### 2.1 Calling convention

- C ABI par défaut (platform default).
- Sur Windows, recommandation : `__cdecl` partout.

### 2.2 Packing

- Interdiction d’utiliser `#pragma pack` pour les structs ABI.
- Alignements naturels.

### 2.3 Endianness

- ABI s’exécute sur l’endianness native.
- Les formats sérialisés (fichiers) doivent spécifier leur endianness séparément.

---

## 3) Types fondamentaux (tailles fixes)

Utiliser `<stdint.h>`.

- `uint8_t/uint16_t/uint32_t/uint64_t`
- `int8_t/int16_t/int32_t/int64_t`
- `uintptr_t/intptr_t`

### 3.1 Bool

- `vitte_bool` = `uint8_t` (0 ou 1)

### 3.2 Handle

- `vitte_handle` = `uint64_t`
- `0` = invalid

Handles typés par convention (noms) mais **même représentation** :
- `vitte_file_handle`
- `vitte_socket_handle`
- `vitte_thread_handle`
- `vitte_vm_object_handle` (objets VM si besoin)

---

## 4) Modèle Result & erreurs

### 4.1 Type code

- `vitte_errc` = `uint32_t`

Codes stables recommandés (alignés PAL) :
- `VITTE_OK = 0`
- `VITTE_EINVAL`
- `VITTE_ENOTSUP`
- `VITTE_ENOMEM`
- `VITTE_EIO`
- `VITTE_EACCES`
- `VITTE_ENOENT`
- `VITTE_EEXIST`
- `VITTE_EBUSY`
- `VITTE_ETIMEDOUT`
- `VITTE_EINTR`
- `VITTE_EAGAIN`
- `VITTE_EPIPE`
- `VITTE_ECLOSED`
- `VITTE_EBADF`
- `VITTE_EOF`
- `VITTE_EOVERFLOW`
- `VITTE_ESTATE`
- `VITTE_EABI` (incompat ABI)

### 4.2 Result

```c
typedef struct {
  vitte_errc code;      /* 0 => ok */
  uint32_t   subcode;   /* optionnel: errno/GetLastError/impl */
} vitte_result;
```

Règle :
- `code == VITTE_OK` ⇒ succès.
- sinon, les sorties associées sont indéfinies (sauf mention contraire).

### 4.3 Variantes typed

Pour éviter les out-params, on peut utiliser des résultats typés :

```c
typedef struct { vitte_result r; uint64_t v; } vitte_u64_result;
typedef struct { vitte_result r; uint32_t v; } vitte_u32_result;
typedef struct { vitte_result r; vitte_bool v; } vitte_bool_result;
```

---

## 5) Slices & buffers

### 5.1 Slice non possédé

```c
typedef struct {
  const uint8_t* ptr;
  uint64_t       len;
} vitte_slice_u8;
```

Sémantique :
- le producteur garantit que `ptr` est valide pendant l’appel.
- le consommateur ne conserve pas `ptr` après retour.

### 5.2 Mutable slice

```c
typedef struct {
  uint8_t*  ptr;
  uint64_t  len;
} vitte_slice_u8_mut;
```

### 5.3 Str (UTF-8) non possédé

```c
typedef struct {
  const char* ptr; /* UTF-8, pas forcément NUL-terminated */
  uint64_t    len;
} vitte_str;
```

Règles :
- UTF-8 valide recommandé (sinon comportement défini par l’API, souvent `EINVAL`).
- `len` est en octets.

### 5.4 StrZ (C-string)

Certaines APIs acceptent un `const char*` NUL-terminated :
- uniquement quand explicitement mentionné.
- encodage UTF-8.

---

## 6) Ownership & allocateurs

### 6.1 Principe : celui qui alloue libère

- Un buffer alloué par le runtime doit être libéré par le runtime.
- Un buffer alloué par un plugin doit être libéré par le plugin.

### 6.2 Allocateur runtime (recommandé)

Le runtime expose une paire :
- `rt_alloc(size, align, *out_ptr)`
- `rt_free(ptr, size, align)`

Sémantique :
- `rt_alloc` retourne un buffer aligné.
- `rt_free` doit être appelé avec la même taille/align.

### 6.3 API “write-into-buffer”

Pour éviter les allocations inter-domaines :
- le caller passe `buf` + `cap`
- l’API écrit et retourne `needed` si `cap` insuffisant.

Pattern :

```c
typedef struct { vitte_result r; uint64_t needed; } vitte_size_result;
```

Règle :
- `VITTE_EOVERFLOW` ou `VITTE_EINVAL` si `cap` incohérent.
- Si `cap < needed`, l’appel retourne `VITTE_EAGAIN` (ou `EOVERFLOW`) et `needed`.

---

## 7) Handles : cycle de vie

### 7.1 Règles générales

- `handle == 0` est invalide.
- Une fonction recevant un handle invalide retourne `VITTE_EBADF`.
- Double fermeture : `VITTE_ECLOSED` ou `VITTE_EBADF` (mais stable recommandé : `ECLOSED`).

### 7.2 Duplication / partage

Optionnel :
- `handle_dup(h, *out)`

Sémantique :
- le runtime peut implémenter un refcount.
- si non supporté, `ENOTSUP`.

---

## 8) ABI plugin (dylib)

### 8.1 Export attendu

Chaque plugin doit exporter une fonction C :

```c
/* nom export stable */
VITTE_EXPORT vitte_result vitte_plugin_entry(const struct vitte_rt_api* rt,
                                             const struct vitte_plugin_desc* desc,
                                             struct vitte_plugin_out* out);
```

- `rt` : pointeur vers l’API runtime (vtable)
- `desc` : infos fournies par le runtime (versions, options)
- `out` : structure que le plugin remplit (vtable plugin, metadata)

### 8.2 Descriptor runtime → plugin

```c
typedef struct {
  uint32_t abi_major, abi_minor, abi_patch;
  uint64_t capabilities; /* runtime caps */
  vitte_str plugin_name; /* optionnel */
} vitte_plugin_desc;
```

### 8.3 Out plugin → runtime

```c
typedef struct {
  uint32_t plugin_abi_major, plugin_abi_minor, plugin_abi_patch;
  uint64_t plugin_caps;
  const void* vtable; /* plugin-defined, append-only */
  vitte_str   name;
  vitte_str   version;
} vitte_plugin_out;
```

Règles :
- Le plugin doit matcher `abi_major`.
- `vtable` est optionnel si le plugin n’expose rien.

---

## 9) API runtime (vtable) — minimum recommandé

Le runtime doit exposer une `vitte_rt_api` append-only.

### 9.1 Version minimale

```c
typedef struct vitte_rt_api {
  /* meta */
  uint32_t abi_major, abi_minor, abi_patch;

  /* alloc */
  vitte_result (*alloc)(uint64_t size, uint64_t align, void** out_ptr);
  vitte_result (*free)(void* ptr, uint64_t size, uint64_t align);

  /* log */
  void (*log)(uint32_t level, vitte_str msg);

  /* errors */
  uint32_t (*last_os_error)(void);

  /* strings helpers (optionnel) */
  vitte_result (*utf8_validate)(vitte_str s);

  /* ... append-only ... */
} vitte_rt_api;
```

Notes :
- `log` doit être safe (ne pas deadlock, éviter allocations).
- `utf8_validate` peut retourner `ENOTSUP` si runtime minimal.

---

## 10) Interop PAL ↔ Runtime

Règle :
- Le plugin ne doit pas appeler directement la PAL.
- Il passe par le runtime (qui arbitre sécurité/caps/policies).

Si un plugin a besoin d’un accès bas niveau (ex: sockets), il le demande via :
- capability runtime
- fonctions dédiées sur la vtable runtime

---

## 11) Stable layouts — recommandations C

### 11.1 Export macro

```c
#if defined(_WIN32)
  #define VITTE_EXPORT __declspec(dllexport)
#else
  #define VITTE_EXPORT __attribute__((visibility("default")))
#endif
```

### 11.2 Static asserts

Toutes les impl doivent vérifier :
- tailles (`sizeof(vitte_handle) == 8`)
- alignements

---

## 12) Exemples

### 12.1 Retour d’une string dans un buffer caller

```c
vitte_size_result rt_get_version(char* buf, uint64_t cap) {
  const char* s = "v0.1.0";
  uint64_t n = 6;
  vitte_size_result out;
  out.needed = n;
  if (cap < n) { out.r.code = VITTE_EAGAIN; out.r.subcode = 0; return out; }
  memcpy(buf, s, (size_t)n);
  out.r.code = VITTE_OK; out.r.subcode = 0;
  return out;
}
```

### 12.2 Plugin entry minimal

```c
VITTE_EXPORT vitte_result vitte_plugin_entry(const vitte_rt_api* rt,
                                             const vitte_plugin_desc* desc,
                                             vitte_plugin_out* out) {
  if (!rt || !desc || !out) return (vitte_result){VITTE_EINVAL, 0};
  if (desc->abi_major != rt->abi_major) return (vitte_result){VITTE_EABI, 0};

  out->plugin_abi_major = desc->abi_major;
  out->plugin_abi_minor = desc->abi_minor;
  out->plugin_abi_patch = desc->abi_patch;
  out->plugin_caps = 0;
  out->vtable = NULL;
  out->name = (vitte_str){"example", 7};
  out->version = (vitte_str){"v0.1.0", 6};

  if (rt->log) rt->log(2 /*info*/, (vitte_str){"plugin loaded", 13});
  return (vitte_result){VITTE_OK, 0};
}
```

---

## 13) Erreurs standard (complément)

- Les erreurs doivent être **stables** : ne pas renvoyer des codes OS directement.
- `subcode` peut contenir l’info OS pour debug.
- Un plugin ne doit pas faire dépendre sa logique métier de `subcode`.

---

## 14) TODO (extensions prévues)

- ABI pour “objects” VM (handles d’objets GC-safe)
- ABI pour callbacks (host→plugin) avec conventions de thread-safety
- ABI streaming (reader/writer interfaces)
- ABI stable pour events/metrics
