# Targets & Triples — matrice OS/arch supportée (référence)

Ce document définit la matrice de **triples** supportés par Vitte et sert de référence pour :
- `muffin` (targets)
- le runtime/VM (PAL)
- la distribution (artefacts `dist/`)

Objectifs :
- Nommer clairement les plateformes **officielles**.
- Distinguer support **Tier 1/2/3**.
- Standardiser les triples au format “type LLVM/Rust”.

Non-objectifs :
- Couvrir toutes les variantes historiques (ex: i386, powerpc) — extensible plus tard.

---

## 1) Définition d’un triple

Un triple canonical s’écrit :

```
<arch>-<vendor>-<os>-<abi>
```

Exemples :
- `aarch64-apple-darwin`
- `x86_64-unknown-linux-gnu`
- `x86_64-pc-windows-msvc`

Remarques :
- `vendor` peut être `unknown`.
- `abi` peut être omis si implicite, mais dans Vitte on recommande de le garder (stabilité).

---

## 2) Tiers de support

### Tier 1 (officiel, CI obligatoire)

- Build & tests automatiques à chaque commit.
- Toolchain documentée.
- Artefacts de release garantis.

### Tier 2 (supporté, CI best-effort)

- Build régulier, tests partiels.
- Artefacts possibles selon releases.

### Tier 3 (expérimental)

- Peut compiler, pas de garantie.
- PRs bienvenues.

---

## 3) Matrice officielle

### 3.1 macOS

Tier 1 :
- `aarch64-apple-darwin` (Apple Silicon)

Tier 2 :
- `x86_64-apple-darwin` (Intel macOS)

Notes :
- Le minimum OS recommandé est macOS 11 pour arm64.
- Universal binaries via builds séparés + `lipo` (voir `flags.md`).

### 3.2 Linux

Tier 1 :
- `x86_64-unknown-linux-gnu`
- `aarch64-unknown-linux-gnu`

Tier 2 :
- `x86_64-unknown-linux-musl`
- `aarch64-unknown-linux-musl`

Notes :
- GNU est la cible par défaut.
- musl est pour builds statiques/containers.

### 3.3 Windows

Tier 1 :
- `x86_64-pc-windows-msvc`

Tier 2 :
- `aarch64-pc-windows-msvc`
- `x86_64-pc-windows-gnu` (LLVM MinGW)

Notes :
- MSVC est recommandé pour end-users.
- MinGW sert aux environnements sans Visual Studio.

### 3.4 *BSD

Tier 2 :
- `x86_64-unknown-freebsd`
- `aarch64-unknown-freebsd`

Tier 3 :
- `x86_64-unknown-openbsd`
- `x86_64-unknown-netbsd`

Notes :
- Les sanitizers peuvent être limités selon OS.

---

## 4) Mapping MUF target

Exemple de `target` MUF basé sur un triple :

```muf
target linux_x64
  triple = "x86_64-unknown-linux-gnu"
.end
```

Règle :
- `target.<name>.triple` doit correspondre à une entrée de cette matrice (sauf mode expérimental).

---

## 5) Policies de build / distribution

### 5.1 Artifacts

Recommandation pour les releases :

- `vittec-<version>-<triple>.tar.gz` (Unix)
- `vittec-<version>-<triple>.zip` (Windows)

Contenu minimal :
- `bin/` (CLI)
- `lib/` (runtime, dylibs)
- `include/` (headers ABI runtime)
- `share/` (completions, docs)

### 5.2 Cross-compilation

- Tier 1 doit être buildable en natif.
- Tier 2 peut dépendre de cross toolchains (ex: musl).
- Cross est piloté par `toolchain` + `target.sysroot` + flags.

---

## 6) Extensions futures

Plates-formes candidates :
- `riscv64gc-unknown-linux-gnu` (Tier 3)
- `wasm32-unknown-unknown` (toolchain spécifique)
- `x86_64-unknown-solaris` (si besoin)

---

## 7) Checklist CI (Tier 1)

Pour Tier 1, la CI doit exécuter :
- build debug + release
- tests runtime/VM
- tests `muffin` (parser MUF + lock)
- smoke tests d’exécution (`vittec --version`, `vittec run examples/hello`)
