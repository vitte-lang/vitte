# TODO: stable PAL contract

# PAL Contract — Platform Abstraction Layer (Runtime)

Ce document définit le **contrat stable** de la PAL (Platform Abstraction Layer) utilisée par le runtime Vitte.

Objectifs :
- Offrir une surface **minimale**, **portable** et **stable** au-dessus des OS (macOS/Linux/Windows/*BSD).
- Isoler le runtime/VM des détails système (threads, time, fs, sockets, virtual memory, dylibs, etc.).
- Garantir un comportement **déterministe** sur les éléments observables (erreurs, tailles, encodages).
- Permettre une implémentation **C99** simple (et/ou Rust) sans dépendances lourdes.

Non-objectifs :
- Fournir une API haut niveau (ça appartient à `std/*`).
- Exposer des primitives “non sûres” sans garde-fous (ex: injection arbitraire de code).

---

## 1) Terminologie

- **PAL** : interface portable (contrat) + implémentations par plateforme.
- **Runtime** : VM, GC, loader, IO bas niveau.
- **Handle** : identifiant opaque pour ressources OS (fichier, socket, thread…).
- **UTF-8** : encodage canonique pour texte au niveau PAL.

---

## 2) Principes généraux

### 2.1 ABI et stabilité

- L’interface est définie en C (C99) sous forme de `struct vitte_pal_api` (table de fonctions).
- Tous les types exposés ont une taille et un alignement stables.
- Aucune exception, aucun longjmp : les erreurs sont retournées par `VittePalResult`.

### 2.2 Initialisation

- Le runtime appelle `pal->init(&cfg, &out_state)` une seule fois.
- Toute autre fonction doit être valide après `init` et avant `shutdown`.
- `shutdown` libère les ressources internes (mais ne ferme pas forcément les handles créés si le runtime les gère).

### 2.3 Encodage & strings

- Toutes les strings côté PAL sont en **UTF-8**.
- Les chemins (`path`) sont en UTF-8 ; l’impl Windows doit convertir en UTF-16 en interne.
- Les strings retournées par la PAL sont soit :
  - écrites dans un buffer fourni par l’appelant, soit
  - retournées via un `pal_alloc/pal_free` (allocateur PAL), jamais via `malloc` direct côté runtime.

### 2.4 Mémoire

- Les fonctions PAL ne doivent pas conserver de pointeur vers des buffers appartenant au runtime après retour.
- Les pages de mémoire virtuelle sont gérées via une API explicite (voir §7).

### 2.5 Concurrence

- Le runtime peut appeler la PAL depuis plusieurs threads.
- Le contrat indique pour chaque sous-API si elle est thread-safe.
- Les callbacks (log, panic, etc.) ne doivent pas provoquer de réentrance non contrôlée.

---

## 3) Modèle d’erreurs

### 3.1 Type résultat

Chaque fonction retourne :

- `VittePalResult` (code + sous-code)
- ou `VittePalBoolResult` / `VittePalU64Result` etc. (valeur + code)

Règle :
- `code == VITTE_PAL_OK` ⇒ succès.
- sinon, la valeur retournée (si présente) est indéfinie.

### 3.2 Codes standards

Codes recommandés (stables) :

- `VITTE_PAL_OK`
- `VITTE_PAL_EINVAL` : argument invalide
- `VITTE_PAL_ENOTSUP` : non supporté sur cette plateforme
- `VITTE_PAL_ENOMEM` : manque mémoire
- `VITTE_PAL_EIO` : erreur I/O générique
- `VITTE_PAL_EACCES` : permission
- `VITTE_PAL_ENOENT` : introuvable
- `VITTE_PAL_EEXIST` : existe déjà
- `VITTE_PAL_EBUSY` : ressource occupée
- `VITTE_PAL_ETIMEDOUT` : timeout
- `VITTE_PAL_EINTR` : interrompu
- `VITTE_PAL_EAGAIN` : réessayer
- `VITTE_PAL_EPIPE` : pipe/socket fermé
- `VITTE_PAL_ECLOSED` : handle déjà fermé
- `VITTE_PAL_EBADF` : handle invalide
- `VITTE_PAL_EOF` : fin de fichier (lecture)
- `VITTE_PAL_EOVERFLOW` : overflow / dépassement
- `VITTE_PAL_ESTATE` : état interne invalide

### 3.3 Mapping OS

- POSIX : mapper errno.
- Windows : mapper `GetLastError()` / `WSAGetLastError()`.
- La PAL peut fournir `last_os_error()` pour debug (non stable pour logique).

---

## 4) Types fondamentaux

### 4.1 Entiers

- `u8/u16/u32/u64`, `i8/i16/i32/i64` : tailles fixes.
- `usize/isize` : taille pointeur.

### 4.2 Handles

- `VittePalHandle` est un entier opaque `u64`.
- `0` est réservé pour "invalid".

Handles typés via tags :
- `VittePalFileHandle`
- `VittePalSocketHandle`
- `VittePalThreadHandle`
- `VittePalMutexHandle`
- etc.

### 4.3 Time

- `VittePalTimeNs` : `u64` nanosecondes.
- `VittePalInstantNs` : monotonic nanosecondes.

---

## 5) API de base

La PAL expose une table de fonctions :

- `pal_version()` : version du contrat
- `capabilities()` : bitset de features supportées
- `init(cfg)` / `shutdown()`
- `alloc(size, align)` / `free(ptr, size, align)` (optionnel)
- `log(level, msg)` (optionnel)

### 5.1 Version

- Version sémantique `vMAJOR.MINOR.PATCH`.
- MAJOR change ⇒ rupture ABI.

### 5.2 Capabilities

Exemples de caps :
- `VITTE_PAL_CAP_THREADS`
- `VITTE_PAL_CAP_SOCKETS`
- `VITTE_PAL_CAP_MMAP`
- `VITTE_PAL_CAP_DYLIB`
- `VITTE_PAL_CAP_CLOCK_MONOTONIC`

---

## 6) Fichiers & FS

### 6.1 I/O fichiers

Opérations minimales :
- `file_open(path, flags)`
- `file_close(h)`
- `file_read(h, buf, len, *out_read)`
- `file_write(h, buf, len, *out_written)`
- `file_seek(h, offset, whence, *out_pos)`
- `file_flush(h)`
- `file_stat(h, *out_stat)`

Flags stables :
- read / write / append / create / truncate / exclusive

### 6.2 FS opérations

- `fs_mkdir(path, recursive)`
- `fs_remove(path, recursive)`
- `fs_rename(src, dst)`
- `fs_copy(src, dst)` (optionnel)
- `fs_read_dir(path, cb, user)` (itérateur)
- `fs_cwd_get(buf)` / `fs_cwd_set(path)`
- `fs_temp_dir(buf)`

### 6.3 Stat

`VittePalFileStat` (stable) :
- `size_bytes: u64`
- `mtime_ns: u64`
- `mode: u32` (bits POSIX-like)
- `type: u8` (file/dir/symlink/other)

Règle : les timestamps sont en **UTC** (mtime) exprimés en ns depuis epoch (Unix epoch).

### 6.4 Chemins

- Le séparateur logique est `/`.
- Sur Windows, la PAL accepte `/` et `\` en input, normalise en interne.

---

## 7) Mémoire virtuelle (VMem)

API optionnelle, mais recommandée pour la VM/GC.

### 7.1 Réservation / commit

- `vmem_page_size(*out)`
- `vmem_reserve(size, *out_ptr)`
- `vmem_commit(ptr, size, prot)`
- `vmem_decommit(ptr, size)`
- `vmem_release(ptr, size)`

Prot :
- `READ`, `WRITE`, `EXEC` (W^X recommandé)

### 7.2 Conventions

- `reserve` retourne une adresse alignée sur la page.
- `commit/decommit` opèrent sur des multiples de page.

---

## 8) Threads & synchro

### 8.1 Threads

- `thread_spawn(entry_fn, user, *out_thread)`
- `thread_join(thread, *out_code)`
- `thread_detach(thread)`
- `thread_current_id(*out)`
- `thread_yield()`

Règle : la fonction `entry_fn` reçoit un pointeur opaque `user`.

### 8.2 Mutex / Cond / Once

- `mutex_create(*out)` / `mutex_lock(h)` / `mutex_unlock(h)` / `mutex_destroy(h)`
- `cond_create(*out)` / `cond_wait(cond, mutex, timeout_ns?)` / `cond_signal(cond)` / `cond_broadcast(cond)` / `cond_destroy(cond)`
- `once(init_fn, *state)` (optionnel)

Timeout :
- `timeout_ns == 0` : poll
- `timeout_ns == UINT64_MAX` : infini

---

## 9) Horloges & temps

- `clock_now_utc_ns(*out)` (wall clock)
- `clock_now_mono_ns(*out)` (monotonic)
- `sleep_ns(duration)`

Règle :
- `mono` ne recule jamais.
- si monotonic non dispo, `ENOTSUP` et fallback runtime.

---

## 10) Réseau (sockets)

Sous-API optionnelle (`VITTE_PAL_CAP_SOCKETS`).

Opérations minimales (TCP/UDP) :
- `sock_open(kind, *out)`
- `sock_close(h)`
- `sock_bind(h, addr)`
- `sock_listen(h, backlog)`
- `sock_accept(h, *out_client, *out_addr)`
- `sock_connect(h, addr, timeout_ns?)`
- `sock_send(h, buf, len, *out_sent)`
- `sock_recv(h, buf, len, *out_read)`
- `sock_set_nonblock(h, bool)`

Address :
- support minimal IPv4.
- IPv6 optionnel.

---

## 11) Process / env

- `env_get(key, buf)` / `env_set(key, value)`
- `args_get(cb, user)` (iteration)
- `proc_exec(path, argv, envp, *out_pid)` (optionnel)
- `proc_wait(pid, timeout_ns?, *out_code)` (optionnel)

Règles :
- Les valeurs sont UTF-8.
- Sur Windows, conversion interne.

---

## 12) Dynamic libraries (dylib)

Sous-API optionnelle (`VITTE_PAL_CAP_DYLIB`).

- `dylib_open(path, *out)`
- `dylib_sym(handle, name, *out_ptr)`
- `dylib_close(handle)`

Règle :
- `name` est ASCII (sym) ; `path` UTF-8.

---

## 13) Random & crypto

Minimum :
- `rng_fill(buf, len)` (CSPRNG OS)

Règle :
- si non disponible : `ENOTSUP` (mais pratiquement dispo sur OS modernes).

---

## 14) Logging, panic, diagnostics

### 14.1 Logging

Optionnel :
- `log(level, msg_utf8)`

Levels :
- trace/debug/info/warn/error

### 14.2 Panic hook

Optionnel :
- `panic(msg_utf8)` : appelé par runtime en cas fatal.

Règle :
- `panic` ne retourne pas (ou si retourne, le runtime abort).

### 14.3 Backtrace

Optionnel :
- `backtrace_capture(buf_frames, *out_len)`
- `backtrace_symbolize(frames, cb)`

---

## 15) Sécurité & sandbox

- Toute API potentiellement dangereuse est **capability-gated**.
- La PAL peut refuser des opérations par policy (`EACCES`).

Exemples :
- `proc_exec` peut être désactivé.
- `dylib_open` peut être désactivé.

---

## 16) Tests de conformité PAL

Chaque implémentation PAL doit fournir un jeu de tests de conformité :

- files : open/read/write/seek/stat
- vmem : reserve/commit/decommit/release
- threads : spawn/join + mutex/cond basiques
- clock : mono monotonic + sleep approx
- rng : non-zero distribution basique

Règle :
- Les tests doivent être runnables via `muffin test --pal`.

---

## 17) Versioning & compatibilité

- `pal_version().major` doit matcher l’attendu du runtime.
- `minor` peut ajouter des fonctions (append-only dans la struct), jamais réordonner.
- `patch` corrige les bugs sans changer le contrat.

---

## 18) Annexe — Schéma C recommandé (informel)

Ce schéma est un guide. La spec ABI exacte vit dans `abi_ffi.md`.

```c
typedef struct {
  uint32_t major, minor, patch;
} vitte_pal_version;

typedef uint64_t vitte_pal_handle;

typedef struct {
  uint32_t code;
  uint32_t subcode;
} vitte_pal_result;

typedef struct vitte_pal_api {
  vitte_pal_version (*pal_version)(void);
  uint64_t (*capabilities)(void);

  vitte_pal_result (*init)(const void* cfg, void** out_state);
  void (*shutdown)(void* state);

  vitte_pal_result (*file_open)(void* st, const char* path_utf8, uint32_t flags, vitte_pal_handle* out);
  vitte_pal_result (*file_close)(void* st, vitte_pal_handle h);
  vitte_pal_result (*file_read)(void* st, vitte_pal_handle h, void* buf, uint64_t len, uint64_t* out_read);
  vitte_pal_result (*file_write)(void* st, vitte_pal_handle h, const void* buf, uint64_t len, uint64_t* out_written);

  /* ... append-only ... */
} vitte_pal_api;
```