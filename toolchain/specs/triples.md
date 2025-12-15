

# Vitte Target Triples (spécification)

Ce document définit la **nomenclature des triples cibles** utilisés par Vitte (style LLVM/Rust), ainsi que les triples supportés et les règles de compat.

Objectifs :
- Une forme **stable** (`arch-vendor-os[-abi]`).
- Un mapping clair vers toolchains externes (clang/llvm, MSVC, GCC, Apple).
- Support cross-compilation et packaging.

---

## Format

Triple canonical :

```text
<arch>-<vendor>-<os>[-<abi>]
```

Exemples :
- `x86_64-unknown-linux-gnu`
- `aarch64-apple-darwin`
- `x86_64-pc-windows-msvc`
- `wasm32-unknown-unknown`

Notes :
- `vendor` peut être `unknown`, `pc`, `apple`.
- `abi` est optionnel selon OS.

---

## Champs

### 1) `arch`

Architectures supportées (phase 1) :
- `x86_64`
- `aarch64`

Architectures planifiées (phase 2/3) :
- `armv7` (ou `armv7a` selon convention choisie)
- `riscv64`
- `wasm32`

### 2) `vendor`

- `unknown` : valeur par défaut cross-platform.
- `pc` : convention Windows.
- `apple` : convention Apple.

### 3) `os`

- `linux`
- `darwin`
- `windows`
- `freebsd`
- `none` (freestanding / baremetal)
- `unknown` (ex: WASM “unknown-unknown”)

### 4) `abi` (optionnel)

Linux :
- `gnu`
- `musl`

Windows :
- `msvc`
- `gnu` (mingw)

Freestanding :
- `elf` (optionnel)

---

## Triples supportés (MVP)

### Linux
- `x86_64-unknown-linux-gnu`
- `x86_64-unknown-linux-musl`
- `aarch64-unknown-linux-gnu`
- `aarch64-unknown-linux-musl`

### macOS
- `x86_64-apple-darwin`
- `aarch64-apple-darwin`

### Windows
- `x86_64-pc-windows-msvc`
- `aarch64-pc-windows-msvc` (si toolchain dispo)
- `x86_64-pc-windows-gnu` (mingw)

### FreeBSD (optionnel)
- `x86_64-unknown-freebsd`
- `aarch64-unknown-freebsd`

---

## Triples “VM” (si backend `vm`)

Si tu conserves un backend VM avec bytecode portable, tu peux définir un pseudo-triple:

- `vittevm-unknown-vm`

ou

- `vm32-unknown-vitte`

Recommandation : garder un identifiant distinct pour éviter confusion avec ABI OS.

---

## Normalisation (alias)

Vitte accepte des alias utilisateurs et les normalise :

| Alias | Canonical |
|------|-----------|
| `linux-x64` | `x86_64-unknown-linux-gnu` |
| `linux-arm64` | `aarch64-unknown-linux-gnu` |
| `mac-x64` | `x86_64-apple-darwin` |
| `mac-arm64` | `aarch64-apple-darwin` |
| `win-x64-msvc` | `x86_64-pc-windows-msvc` |
| `win-x64-gnu` | `x86_64-pc-windows-gnu` |

Règles :
- Si l’utilisateur passe `--target x86_64-linux`, compléter en `x86_64-unknown-linux-gnu`.
- Si `abi` absent sur Linux, défaut `gnu` (ou `musl` si profil `static`/`bundle` le demande).

---

## Détection host

L’outil `vitte` doit pouvoir détecter le host triple :
- macOS arm64 → `aarch64-apple-darwin`
- macOS x64 → `x86_64-apple-darwin`
- Linux → `x86_64-unknown-linux-gnu` ou `aarch64-unknown-linux-gnu`
- Windows → `x86_64-pc-windows-msvc` (par défaut) si MSVC présent, sinon `x86_64-pc-windows-gnu`

Le host est exposé via :

```sh
vitte --print host
vitte --print target
```

---

## Compat et contraintes

### Cross-compilation

- `--target <triple>` impose :
  - sysroot (si nécessaire)
  - linker/toolchain correspondants
  - libs systèmes pour la cible

Vitte doit valider :
- `arch` supporté
- `os` supporté
- `abi` cohérent

### Linker / toolchain mapping

- `*-windows-msvc` → `link.exe` / lld-link
- `*-windows-gnu` → `ld` (mingw) / lld
- `*-apple-darwin` → `clang` + `ld64` / `ld64.lld`
- `*-linux-gnu` → `clang/gcc` + `ld.lld/ld`
- `*-linux-musl` → musl toolchain + sysroot

---

## Extensions et conventions de sortie

La convention des extensions dépend du `os` :
- exe : `.exe` uniquement sur Windows
- shared lib : `.so` (linux), `.dylib` (mac), `.dll` (win)
- static lib : `.a` (linux/mac), `.lib` (win)

---

## Exemples

### Build natif

```sh
vitte build --profile dev
```

### Cross Linux → macOS arm64

```sh
vitte build --target aarch64-apple-darwin --profile release
```

### Cross vers musl

```sh
vitte build --target x86_64-unknown-linux-musl --profile release --static
```

---

## Notes d’implémentation

- Parser triple :
  - split `-` en 3 ou 4 segments
  - validate `arch`, `vendor`, `os`, `abi`
  - apply alias normalization

- Stocker les triples dans `toolchain/targets/triples.c` (ou équivalent), export API :
  - `vitte_triple_parse`
  - `vitte_triple_normalize`
  - `vitte_triple_host_detect`
