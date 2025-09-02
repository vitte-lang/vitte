# Vitte RFCs — Index & Guide

> Une seule boussole : faire de Vitte un langage net, rapide, fiable. Le processus RFC est là pour cadrer les idées, pas pour les étouffer. ✨

---

## Comment fonctionne le processus RFC

1. **Ouvrir une proposition** : copiez `0000-template.md` → `NNNN-titre-kebab.md` (NNNN = numéro, 4 chiffres).
2. **Rédiger** : remplissez les sections (motivation, design, alternatives, compat…).
3. **Discuter** : PR GitHub, reviews techniques, itérations.
4. **Statuts** :

   * 🍼 *Draft* — en cours d’écriture.
   * 🗳️ *Proposed* — prête pour review sérieuse.
   * ✅ *Accepted* — plan d’implémentation OK.
   * 🔧 *In Progress* — en cours d’implémentation.
   * 🚢 *Stabilized* — livré et gelé.
   * ❌ *Rejected* — fermé avec rationale.
5. **Numérotation** : séquentielle. Évitez les collisions ; si ça arrive, renumérotez dans la PR.

> **Astuce** (Windows / PowerShell) :
>
> ```powershell
> gci rfcs -Filter "*.md" | % { $_.Name } | \
>  ? { $_ -match '^\d{4}-' } | \
>  % { $_.Substring(0,4) } | group | ? Count -gt 1
> ```

---

## Conflits de numéros à résoudre

Plusieurs fichiers partagent le même numéro (A/B). À renuméroter lors des prochaines PR :

* **0020** : `governance-and-evolution` **&** `internationalization-and-localization`
* **0021** : `garbage-collection-vs-manual-memory` **&** `internationalization-localization`
* **0022** : `embedded-systems-programming` **&** `reflection-and-metaprogramming`
* **0023** : `serialization-and-deserialization` **&** `webassembly-web-integration`
* **0024** : `ai-ml-data-science` **&** `networking-and-io`
* **0025** : `networking-distributed-systems` **&** `parallelism-and-simd`
* **0026** : `mobile-cross-platform` **&** `webassembly-and-embedded-support`
* **0027** : `game-development-graphics` **&** `versioning-and-semver-policy`
* **0028** : `blockchain-smart-contracts` **&** `governance-and-rfc-process`
* **0029** : `operating-system-development` **&** `standard-error-codes-and-diagnostics`
* **0030** : `debugging-and-profiling-tools` **&** `quantum-computing-future`

> Proposition : renuméroter les entrées « B » à partir de **0054** (prochain slot libre) en conservant l’ordre ci‑dessus.

---

## Index global (par numéro)

> Statut initial : 🍼 *Draft* partout, sauf mention contraire.

|      # | Titre                               | Fichier                                         | Statut   |
| -----: | ----------------------------------- | ----------------------------------------------- | -------- |
|   0000 | **Template**                        | `0000-template.md`                              | –        |
|   0001 | Core Syntax & Keywords              | `0001-core-syntax-and-keywords.md`              | 🍼 Draft |
|   0002 | Module System                       | `0002-module-system.md`                         | 🍼 Draft |
|   0003 | Memory Model & Ownership            | `0003-memory-model-and-ownership.md`            | 🍼 Draft |
|   0004 | Error Handling                      | `0004-error-handling.md`                        | 🍼 Draft |
|   0005 | FFI & Interoperability              | `0005-ffi-and-interoperability.md`              | 🍼 Draft |
|   0006 | Async/Await & Concurrency           | `0006-async-await-concurrency.md`               | 🍼 Draft |
|   0007 | Pattern Matching                    | `0007-pattern-matching.md`                      | 🍼 Draft |
|   0008 | Macro System                        | `0008-macro-system.md`                          | 🍼 Draft |
|   0009 | Standard Library Structure          | `0009-std-library-structure.md`                 | 🍼 Draft |
|   0010 | Package Manager (vitpm)             | `0010-package-manager-vitpm.md`                 | 🍼 Draft |
|   0011 | Compiler Architecture               | `0011-compiler-architecture.md`                 | 🍼 Draft |
|   0012 | Bytecode & VM                       | `0012-bytecode-and-vm.md`                       | 🍼 Draft |
|   0013 | Security Threat Model               | `0013-security-threat-model.md`                 | 🍼 Draft |
|   0014 | Tooling Integration                 | `0014-tooling-integration.md`                   | 🍼 Draft |
|   0015 | IDE & LSP Support                   | `0015-ide-and-lsp-support.md`                   | 🍼 Draft |
|   0016 | Build System & CI                   | `0016-build-system-and-ci.md`                   | 🍼 Draft |
|   0017 | Testing & Benchmarking              | `0017-testing-and-benchmarking.md`              | 🍼 Draft |
|   0018 | Formatting Style & Lints            | `0018-formatting-style-and-lints.md`            | 🍼 Draft |
|   0019 | Distribution & Installation         | `0019-distribution-and-installation.md`         | 🍼 Draft |
| 0020 A | Governance & Evolution              | `0020-governance-and-evolution.md`              | 🍼 Draft |
| 0020 B | Internationalization & Localization | `0020-internationalization-and-localization.md` | 🍼 Draft |
| 0021 A | Garbage Collection vs Manual Memory | `0021-garbage-collection-vs-manual-memory.md`   | 🍼 Draft |
| 0021 B | Internationalization / Localization | `0021-internationalization-localization.md`     | 🍼 Draft |
| 0022 A | Embedded Systems Programming        | `0022-embedded-systems-programming.md`          | 🍼 Draft |
| 0022 B | Reflection & Metaprogramming        | `0022-reflection-and-metaprogramming.md`        | 🍼 Draft |
| 0023 A | Serialization & Deserialization     | `0023-serialization-and-deserialization.md`     | 🍼 Draft |
| 0023 B | WebAssembly & Web Integration       | `0023-webassembly-web-integration.md`           | 🍼 Draft |
| 0024 A | AI/ML & Data Science                | `0024-ai-ml-data-science.md`                    | 🍼 Draft |
| 0024 B | Networking & I/O                    | `0024-networking-and-io.md`                     | 🍼 Draft |
| 0025 A | Networking & Distributed Systems    | `0025-networking-distributed-systems.md`        | 🍼 Draft |
| 0025 B | Parallelism & SIMD                  | `0025-parallelism-and-simd.md`                  | 🍼 Draft |
| 0026 A | Mobile & Cross‑Platform             | `0026-mobile-cross-platform.md`                 | 🍼 Draft |
| 0026 B | WebAssembly & Embedded Support      | `0026-webassembly-and-embedded-support.md`      | 🍼 Draft |
| 0027 A | Game Development & Graphics         | `0027-game-development-graphics.md`             | 🍼 Draft |
| 0027 B | Versioning & SemVer Policy          | `0027-versioning-and-semver-policy.md`          | 🍼 Draft |
| 0028 A | Blockchain & Smart Contracts        | `0028-blockchain-smart-contracts.md`            | 🍼 Draft |
| 0028 B | Governance & RFC Process            | `0028-governance-and-rfc-process.md`            | 🍼 Draft |
| 0029 A | Operating System Development        | `0029-operating-system-development.md`          | 🍼 Draft |
| 0029 B | Standard Error Codes & Diagnostics  | `0029-standard-error-codes-and-diagnostics.md`  | 🍼 Draft |
| 0030 A | Debugging & Profiling Tools         | `0030-debugging-and-profiling-tools.md`         | 🍼 Draft |
| 0030 B | Quantum Computing — Future          | `0030-quantum-computing-future.md`              | 🍼 Draft |
|   0031 | Energy Efficiency & Green Computing | `0031-energy-efficiency-green-computing.md`     | 🍼 Draft |
|   0032 | AI/ML Integration                   | `0032-ai-ml-integration.md`                     | 🍼 Draft |
|   0033 | Augmented/Virtual Reality           | `0033-augmented-reality-virtual-reality.md`     | 🍼 Draft |
|   0034 | Space Computing & Aerospace         | `0034-space-computing-aerospace.md`             | 🍼 Draft |
|   0035 | Blockchain, Aerospace & Defense     | `0035-blockchain-aerospace-defense.md`          | 🍼 Draft |
|   0036 | Cybersecurity Framework             | `0036-cybersecurity-framework.md`               | 🍼 Draft |
|   0037 | Edge Computing & IoT Security       | `0037-edge-computing-iot-security.md`           | 🍼 Draft |
|   0038 | Neural Interfaces / BCI             | `0038-neural-interfaces-bci.md`                 | 🍼 Draft |
|   0039 | Genomics & Bioinformatics           | `0039-genomics-bioinformatics.md`               | 🍼 Draft |
|   0040 | Quantum Communication               | `0040-quantum-communication.md`                 | 🍼 Draft |
|   0041 | Green Computing                     | `0041-green-computing.md`                       | 🍼 Draft |
|   0042 | Ethical AI                          | `0042-ethical-ai.md`                            | 🍼 Draft |
|   0043 | Robotics & Autonomous Systems       | `0043-robotics-autonomous.md`                   | 🍼 Draft |
|   0044 | Smart Cities                        | `0044-smart-cities.md`                          | 🍼 Draft |
|   0045 | Space Colonization                  | `0045-space-colonization.md`                    | 🍼 Draft |
|   0046 | Metaverse / VR                      | `0046-metaverse-vr.md`                          | 🍼 Draft |
|   0047 | Blockchain dApps                    | `0047-blockchain-dapps.md`                      | 🍼 Draft |
|   0048 | Neuromorphic Computing              | `0048-neuromorphic.md`                          | 🍼 Draft |
|   0049 | Digital Democracy                   | `0049-digital-democracy.md`                     | 🍼 Draft |
|   0050 | Posthuman Computing                 | `0050-posthuman-computing.md`                   | 🍼 Draft |
|   0051 | Exascale Computing                  | `0051-exascale-computing.md`                    | 🍼 Draft |
|   0052 | Synthetic Biology                   | `0052-synthetic-biology.md`                     | 🍼 Draft |
|   0053 | Interstellar Networks               | `0053-interstellar-networks.md`                 | 🍼 Draft |

**Prochain numéro libre** : `0054`.

---

## Index thématique (pour se repérer vite)

### Langage & Sémantique

* 0001 Core Syntax • 0007 Pattern Matching • 0008 Macros • 0003 Memory Model • 0004 Errors
* 0021A GC vs Manual • 0022B Reflection • 0012 Bytecode/VM • 0011 Compiler Architecture

### Écosystème & Outils

* 0010 vitpm • 0014 Tooling • 0015 IDE/LSP • 0016 CI/Build • 0017 Tests/Bench • 0018 Formatting
* 0030A Debug/Profiling • 0027B SemVer/Versioning

### Modules/Interop & Plateformes

* 0002 Modules • 0005 FFI • 0023A Serialization • 0023B WASM/Web • 0026A Mobile • 0026B WASM+Embedded
* 0024B Net & I/O • 0025A Networking/Distributed • 0025B SIMD/Parallel

### Gouvernance & Process

* 0028B RFC Process • 0020A Governance • 0019 Distribution • 0027B Versioning

### Horizons & Recherche

* 0031 Green/Energy • 0041 Green • 0032 AI/ML Integration • 0024A AI/DS • 0042 Ethical AI
* 0038 BCI • 0039 Genomics • 0040 Quantum Comm • 0030B Quantum Future • 0034/0045 Space

---

## Lien / Navigation

* Modèle : [`0000-template.md`](0000-template.md) • **À cloner** pour les nouvelles propositions.
* Index lui‑même : `rfcs/index.md` (vous êtes ici).
* Ancien modèle : `TEMPLATE.md` (legacy).

---

## Choses à faire (check‑list mainteneur)

* [ ] Renuméroter les doublons → 0054+.
* [ ] Ajouter une *front‑matter* YAML standard (title, authors, status, created, updated) à chaque RFC.
* [ ] Brancher l’index dans la doc du site.
* [ ] CI : valider unicité des numéros + titres non vides.

> « Construisons lentement, proprement, et livrons vite. » — Le Mantra Vitte 💙
