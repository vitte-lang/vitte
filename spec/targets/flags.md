# TODO: flags per OS/toolchain

# Flags & toolchains — matrice OS/arch (référence)

Ce document liste les **flags recommandés** par OS/toolchain pour construire :
- le runtime Vitte (C99)
- les outils (CLI)
- les modules natifs (dylibs/plugins)

Objectifs :
- Defaults **sûrs** et **portables**.
- Différencier : debug/release, sanitizers, LTO.
- Avoir une base pour `muffin` (targets/profiles).

Non-objectifs :
- Remplacer une doc complète clang/gcc/msvc.

---

## 0) Conventions

### 0.1 Variables

- `${CC}` : compilateur C (clang/gcc/cl)
- `${CFLAGS}` : flags compile
- `${LDFLAGS}` : flags link

### 0.2 Profils MUF (rappel)

- `profile debug` : opt 0, debug true
- `profile release` : opt 3, debug false

### 0.3 Politique warnings

Recommandation pour code C runtime :
- warnings élevés
- warnings fatals **en CI** (pas forcément sur machine dev)

---

## 1) Flags communs (toutes plateformes)

### 1.1 C standard

- `-std=c99`

### 1.2 Warnings (base)

- clang/gcc :
  - `-Wall -Wextra -Wpedantic`
  - `-Wshadow -Wconversion -Wsign-conversion` (optionnel, plus strict)
  - `-Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition` (runtime C pur)

### 1.3 Définitions

- `-DVITTE_BUILD` (toujours)
- `-DVITTE_DEBUG` (debug)

### 1.4 Optimisation

- debug : `-O0`
- release : `-O3` (ou `-O2` si taille/temps compile)
- size : `-Os` / `-Oz` (clang)

---

## 2) macOS (Apple Clang)

### 2.1 CFLAGS

Debug :
- `-g -O0`
- `-fno-omit-frame-pointer`

Release :
- `-O3`
- `-fvisibility=hidden`
- `-fno-omit-frame-pointer` (recommandé pour perf profiling)

Général :
- `-mmacosx-version-min=11.0` (adapter selon support)

### 2.2 LDFLAGS

- `-Wl,-dead_strip` (release)
- `-Wl,-rpath,@loader_path` (dylibs/plugins si besoin)

### 2.3 Sanitizers

ASan/UBSan :
- `-fsanitize=address,undefined`
- `-fno-omit-frame-pointer`

Notes :
- ASan sur macOS impose souvent une toolchain clang récente.

### 2.4 Arch

- arm64 : `-arch arm64`
- x86_64 : `-arch x86_64`

Universal (fat) :
- build séparé puis `lipo` (recommandé)

---

## 3) Linux (clang / gcc)

### 3.1 CFLAGS

Debug :
- `-g -O0`
- `-fno-omit-frame-pointer`

Release :
- `-O3`
- `-fvisibility=hidden`

Fortify/hardening (recommandé) :
- `-D_FORTIFY_SOURCE=2` (avec `-O2`)
- `-fstack-protector-strong`
- `-fPIC` (dylibs)

### 3.2 LDFLAGS

- `-Wl,--as-needed`
- `-Wl,--gc-sections` (si compile avec `-ffunction-sections -fdata-sections`)
- `-Wl,-rpath,$ORIGIN` (plugins)

### 3.3 Sanitizers

ASan/UBSan :
- `-fsanitize=address,undefined`
- `-fno-omit-frame-pointer`

TSan :
- `-fsanitize=thread`

### 3.4 Musl

Si cible musl :
- triple : `x86_64-unknown-linux-musl`
- éviter certaines options glibc spécifiques.

---

## 4) Windows

Deux familles :
- MSVC (cl/link)
- LLVM MinGW (clang + lld + mingw)

### 4.1 Windows / MSVC

#### CFLAGS (cl)

Debug :
- `/Zi /Od /RTC1`

Release :
- `/O2 /GL`

Warnings :
- `/W4` (base)
- `/WX` (CI)

Runtime :
- `/MDd` (debug) / `/MD` (release) (CRT dynamique) — recommandé.

#### LDFLAGS (link)

- `/DEBUG` (debug)
- `/LTCG` (release si `/GL`)

Dylib :
- `/DLL`

#### Sanitizers

- ASan est supporté sur clang-cl plus facilement que cl (selon versions).

### 4.2 Windows / LLVM MinGW

#### CFLAGS

- `-DWIN32_LEAN_AND_MEAN -DNOMINMAX`
- Debug : `-g -O0`
- Release : `-O3 -fvisibility=hidden`

#### LDFLAGS

- `-Wl,--gc-sections` (si `-ffunction-sections -fdata-sections`)

---

## 5) FreeBSD / OpenBSD / NetBSD

### 5.1 CFLAGS

- clang est souvent le default.
- Debug : `-g -O0 -fno-omit-frame-pointer`
- Release : `-O3 -fvisibility=hidden`

### 5.2 LDFLAGS

- similaires Linux, mais vérifier `--gc-sections` (support linker)

### 5.3 Notes

- Sur OpenBSD, certaines options de sanitizer peuvent être indisponibles.

---

## 6) Flags pour dylibs / plugins

### 6.1 Symbol visibility

Recommandé :
- compiler tout en hidden : `-fvisibility=hidden`
- exporter explicitement les symboles ABI (ex: `VITTE_EXPORT`)

### 6.2 PIC

- Linux/*BSD : `-fPIC`
- macOS : généralement implicite pour dylib, mais ok

### 6.3 RPath

- Linux : `-Wl,-rpath,$ORIGIN`
- macOS : `-Wl,-rpath,@loader_path`

---

## 7) LTO

### 7.1 Clang/GCC

- ThinLTO clang : `-flto=thin`
- Full LTO : `-flto`

Notes :
- nécessite `ar/ranlib` compatibles.

### 7.2 MSVC

- `/GL` + `/LTCG`

---

## 8) Debugging & diagnostics

Recommandé :
- `-g` (DWARF sur Unix)
- `-fno-omit-frame-pointer`
- `-fno-optimize-sibling-calls` (debug deep stacks)

Sur MSVC :
- `/Zi` + `/DEBUG`

---

## 9) Recommandations “release hardened”

Linux (exemple) :
- `-O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIC`
- Link : `-Wl,-z,relro -Wl,-z,now`

macOS :
- `-O3 -fvisibility=hidden -Wl,-dead_strip`

Windows :
- MSVC release : `/O2 /GL` + `/LTCG`

---

## 10) Mapping MUF target/profile (exemple)

```muf
target linux_x64
  triple = "x86_64-unknown-linux-gnu"
  cflags
    - "-std=c99"
    - "-Wall"
    - "-Wextra"
    - "-Wpedantic"
    - "-fvisibility=hidden"
  .end
  ldflags
    - "-Wl,--as-needed"
  .end
.end

profile release
  opt 3
  debug false
  lto thin
.end
```