```
├── .github
│   ├── wotkflows
│   │   ├── license-check.yml
│   │   ├── ci.yml
├── benchmarks
│   ├── baselines
│   │   ├── macro
│   │   │   ├── sample.csv
│   │   ├── micro
│   │   │   ├── sample.json
│   ├── macro
│   │   ├── programs
│   │   │   ├── echo.vitte
│   │   │   ├── fib.vitte
│   │   │   ├── json_parse.vitte
│   │   │   ├── primes.vitte
│   │   ├── tools
│   │   │   ├── run-macro.sh
│   │   ├── scenarios.json
│   ├── micro
│   │   ├── .cargo
│   │   │   ├── config.toml
│   │   ├── benches
│   │   │   ├── bytecode_ops.rs
│   │   │   ├── channels.rs
│   │   │   ├── collections.rs
│   │   │   ├── parser.rs
│   │   │   ├── shims.rs
│   │   │   ├── tokenizer.rs
│   │   │   ├── vm_stack.rs
│   │   ├── Cargo.toml
│   ├── perf
│   │   ├── README.md
│   ├── scripts
│   │   ├── capture-env.sh
│   │   ├── compare.py
│   ├── README.md
├── crates
│   ├── stdlib
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-ast
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-cli
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitte-compiler
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-core
│   │   ├── src
│   │   │   ├── chunk.rs
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-dap
│   │   ├── src
│   │   │   ├── dap.rs
│   │   │   ├── handler.rs
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitte-debugger
│   │   ├── src
│   │   │   ├── api.rs
│   │   │   ├── breakpoint.rs
│   │   │   ├── eval.rs
│   │   │   ├── hook.rs
│   │   │   ├── mod.rs
│   │   │   ├── state.rs
│   │   │   ├── step.rs
│   │   ├── Cargo.toml
│   ├── vitte-fuzz
│   │   ├── corpora
│   │   │   ├── asm
│   │   │   │   ├── sample.asm
│   │   │   ├── csv
│   │   │   │   ├── basic.csv
│   │   │   ├── disasm
│   │   │   │   ├── junk.bin
│   │   │   ├── ini
│   │   │   │   ├── basic.ini
│   │   │   ├── loader
│   │   │   │   ├── sample.vbc
│   │   │   ├── parser
│   │   │   │   ├── arith.vitte
│   │   │   ├── pretty
│   │   │   │   ├── strings.vitte
│   │   │   ├── tokenizer
│   │   │   │   ├── hello.vitte
│   │   │   ├── vm
│   │   │   │   ├── loop.vitte
│   │   │   ├── wasm_emitter
│   │   │   │   ├── print.vitte
│   │   ├── dicts
│   │   │   ├── vitte.dict
│   │   ├── fuzz_targets
│   │   │   ├── _do_not_run.rs
│   │   │   ├── asm.rs
│   │   │   ├── csv.rs
│   │   │   ├── disasm.rs
│   │   │   ├── ini.rs
│   │   │   ├── loader.rs
│   │   │   ├── parser.rs
│   │   │   ├── pretty.rs
│   │   │   ├── tokenizer.rs
│   │   │   ├── vm.rs
│   │   │   ├── wasm_emitter.rs
│   │   ├── scripts
│   │   │   ├── run-all.sh
│   │   ├── shims
│   │   │   ├── lib.rs
│   │   ├── asm.options
│   │   ├── Cargo.toml
│   │   ├── csv.options
│   │   ├── disasm.options
│   │   ├── ini.options
│   │   ├── loader.options
│   │   ├── parser.options
│   │   ├── pretty.options
│   │   ├── README.md
│   │   ├── tokenizer.options
│   │   ├── vm.options
│   │   ├── wasm_emitter.options
│   ├── vitte-ir
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-lexer
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-lsp
│   │   ├── src
│   │   │   ├── lib.rs
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitte-parser
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-runtime
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-tools
│   │   ├── src
│   │   │   ├── bin
│   │   │   │   ├── vitte-asm.rs
│   │   │   │   ├── vitte-disasm.rs
│   │   │   │   ├── vitte-fmt.rs
│   │   │   │   ├── vitte-link.rs
│   │   │   │   ├── vitte-repl.rs
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-vitbc
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-vm
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-wasm
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
├── desktop
│   ├── assets
│   │   ├── icons
│   │   │   ├── vitte-desktop.svg
│   ├── gtk
│   ├── packaging
│   │   ├── linux
│   │   │   ├── vitte-studio.desktop
│   │   ├── mac
│   │   │   ├── Info.plist
│   │   ├── win
│   │   │   ├── vitte-studio.nsi
│   ├── qt
│   │   ├── src
│   │   │   ├── main.cpp
│   │   ├── CMakeLists.txt
│   ├── scripts
│   │   ├── build-gtk.sh
│   │   ├── build-qt.sh
│   ├── gtk_real.c
│   ├── gtk_stub.c
│   ├── main.vitte
│   ├── Makefile
│   ├── qt_real.cpp
│   ├── qt_stub.cpp
│   ├── README.md
├── docs
│   ├── assets
│   │   ├── branding
│   │   │   ├── brand-guidelines.md
│   │   │   ├── LICENSE
│   │   │   ├── palette.json
│   │   ├── fonts
│   │   │   ├── README.md
│   │   ├── icons
│   │   │   ├── book.svg
│   │   │   ├── branch.svg
│   │   │   ├── bug.svg
│   │   │   ├── bytecode.svg
│   │   │   ├── chip.svg
│   │   │   ├── cloud.svg
│   │   │   ├── compiler.svg
│   │   │   ├── cpu.svg
│   │   │   ├── cube.svg
│   │   │   ├── docs.svg
│   │   │   ├── globe.svg
│   │   │   ├── lock.svg
│   │   │   ├── memory.svg
│   │   │   ├── merge.svg
│   │   │   ├── package.svg
│   │   │   ├── pause.svg
│   │   │   ├── pen.svg
│   │   │   ├── play.svg
│   │   │   ├── plug.svg
│   │   │   ├── rocket.svg
│   │   │   ├── shield.svg
│   │   │   ├── stack.svg
│   │   │   ├── stop.svg
│   │   │   ├── vm.svg
│   │   │   ├── wand.svg
│   │   ├── illustrations
│   │   │   ├── backgrounds
│   │   │   │   ├── pattern.svg
│   │   │   ├── banners
│   │   │   │   ├── social-card.svg
│   │   │   ├── hero.svg
│   │   ├── logo
│   │   │   ├── browserconfig.xml
│   │   │   ├── favicon.svg
│   │   │   ├── site.webmanifest
│   │   │   ├── vitte-logo-dark.svg
│   │   │   ├── vitte-logo-light.svg
│   │   │   ├── vitte-mark.svg
│   │   │   ├── vitte-wordmark.svg
│   │   ├── screenshots
│   │   │   ├── ide-mock-dark.svg
│   │   │   ├── ide-mock-light.svg
│   │   ├── templates
│   │   │   ├── badges
│   │   │   │   ├── bench.svg
│   │   │   │   ├── build.svg
│   │   │   │   ├── tests.svg
│   │   │   ├── slide-cover.svg
│   │   ├── README.md
│   ├── diagrams
│   │   ├── drawio
│   │   │   ├── compiler.drawio
│   │   │   ├── vm.drawio
│   │   ├── mermaid
│   │   │   ├── compiler.mmd
│   │   │   ├── README.md
│   │   │   ├── vm.mmd
│   │   ├── bytecode-format.svg
│   │   ├── compiler-pipeline.svg
│   │   ├── lsp-flow.svg
│   │   ├── memory-model.svg
│   │   ├── module-system.svg
│   │   ├── runtime-lifecycle.svg
│   │   ├── vm-architecture.svg
│   │   ├── wasm-pipeline.svg
│   ├── arborescence.md
│   ├── build-from-source.md
│   ├── code-style.md
│   ├── contributing.md
│   ├── ffi.md
│   ├── getting-starte.md
│   ├── index.md
│   ├── language-spec.md
│   ├── stdlib.md
├── examples
│   ├── embedded-blink
│   │   ├── boards
│   │   │   ├── esp32
│   │   │   │   ├── CMakeLists.txt
│   │   │   │   ├── main.c
│   │   │   ├── posix
│   │   │   │   ├── Makefile
│   │   │   │   ├── runner.c
│   │   │   ├── rp2040
│   │   │   │   ├── CMakeLists.txt
│   │   │   │   ├── vitte_ffi_rp2040.c
│   │   │   ├── stm32f4
│   │   │   │   ├── gpio.c
│   │   │   │   ├── linker.ld
│   │   │   │   ├── Makefile
│   │   │   │   ├── start.S
│   │   ├── docs
│   │   │   ├── ffi-contract.md
│   │   │   ├── ports.md
│   │   ├── examples
│   │   │   ├── pwm.vitte
│   │   │   ├── sos.vitte
│   │   ├── ffi
│   │   │   ├── vitte_ffi.h
│   │   ├── scripts
│   │   │   ├── build-esp32.sh
│   │   │   ├── build-posix.sh
│   │   │   ├── build-rp2040.sh
│   │   │   ├── build-stm32f4.sh
│   │   ├── main.vitte
│   │   ├── README.md
│   ├── hello
│   │   ├── src
│   │   │   ├── main.vit
│   │   ├── vitte.toml
│   ├── hello-vitte
│   │   ├── main.vitte
│   ├── kernel
│   │   ├── armv7em
│   │   │   ├── kmain.vitte
│   │   │   ├── linker.ld
│   │   │   ├── start.S
│   │   ├── x86_64
│   │   │   ├── kmain.vitte
│   │   │   ├── linker.ld
│   │   │   ├── start.S
│   │   ├── README.md
│   ├── wasm-add
│   │   ├── main.vitte
│   ├── web-echo
│   │   ├── main.vitte
│   │   ├── middleware.vitte
│   │   ├── README.md
│   ├── worker-jobs
├── rfcs
│   ├── 0000-template.md
│   ├── 0001-core-syntax-and-keywords.md
│   ├── 0002-module-system.md
│   ├── 0003-memory-model-and-ownership.md
│   ├── 0004-error-handling.md
│   ├── 0005-ffi-and-interoperability.md
│   ├── 0006-async-await-concurrency.md
│   ├── 0007-pattern-matching.md
│   ├── 0008-macro-system.md
│   ├── 0009-std-library-structure.md
│   ├── 0010-package-manager-vitpm.md
│   ├── 0011-compiler-architecture.md
│   ├── 0012-bytecode-and-vm.md
│   ├── 0013-security-threat-model.md
│   ├── 0014-tooling-integration.md
│   ├── 0015-ide-and-lsp-support.md
│   ├── 0016-build-system-and-ci.md
│   ├── 0017-testing-and-benchmarking.md
│   ├── 0018-formatting-style-and-lints.md
│   ├── 0019-distribution-and-installation.md
│   ├── 0020-governance-and-evolution.md
│   ├── 0020-internationalization-and-localization.md
│   ├── 0021-garbage-collection-vs-manual-memory.md
│   ├── 0021-internationalization-localization.md
│   ├── 0022-embedded-systems-programming.md
│   ├── 0022-reflection-and-metaprogramming.md
│   ├── 0023-serialization-and-deserialization.md
│   ├── 0023-webassembly-web-integration.md
│   ├── 0024-ai-ml-data-science.md
│   ├── 0024-networking-and-io.md
│   ├── 0025-networking-distributed-systems.md
│   ├── 0025-parallelism-and-simd.md
│   ├── 0026-mobile-cross-platform.md
│   ├── 0026-webassembly-and-embedded-support.md
│   ├── 0027-game-development-graphics.md
│   ├── 0027-versioning-and-semver-policy.md
│   ├── 0028-blockchain-smart-contracts.md
│   ├── 0028-governance-and-rfc-process.md
│   ├── 0029-operating-system-development.md
│   ├── 0029-standard-error-codes-and-diagnostics.md
│   ├── 0030-debugging-and-profiling-tools.md
│   ├── 0030-quantum-computing-future.md
│   ├── 0031-energy-efficiency-green-computing.md
│   ├── 0032-ai-ml-integration.md
│   ├── 0033-augmented-reality-virtual-reality.md
│   ├── 0034-space-computing-aerospace.md
│   ├── 0035-blockchain-aerospace-defense.md
│   ├── 0036-cybersecurity-framework.md
│   ├── 0037-edge-computing-iot-security.md
│   ├── 0038-neural-interfaces-bci.md
│   ├── 0039-genomics-bioinformatics.md
│   ├── 0040-quantum-communication.md
│   ├── 0041-green-computing.md
│   ├── 0042-ethical-ai.md
│   ├── 0043-robotics-autonomous.md
│   ├── 0044-smart-cities.md
│   ├── 0045-space-colonization.md
│   ├── 0046-metaverse-vr.md
│   ├── 0047-blockchain-dapps.md
│   ├── 0048-neuromorphic.md
│   ├── 0049-digital-democracy.md
│   ├── 0050-posthuman-computing.md
│   ├── 0051-exascale-computing.md
│   ├── 0052-synthetic-biology.md
│   ├── 0053-interstellar-networks.md
│   ├── index.md
│   ├── TEMPLATE.md
├── scripts
│   ├── utils
│   ├── bench.sh
│   ├── build.sh
│   ├── ci.sh
│   ├── dev-env.sh
│   ├── doc.sh
│   ├── fmt.sh
│   ├── fuzz.sh
│   ├── lint.sh
│   ├── release.sh
│   ├── test.sh
├── security
│   ├── policy.md
│   ├── threat-model.md
├── std
│   ├── docs
│   │   ├── stdlib-extended.md
│   ├── modules
│   │   ├── algo
│   │   │   ├── dsu.vitte
│   │   │   ├── graph.vitte
│   │   │   ├── heap.vitte
│   │   │   ├── search.vitte
│   │   │   ├── sort.vitte
│   │   │   ├── trie.vitte
│   │   ├── audio
│   │   │   ├── mp3.vitte
│   │   │   ├── wav.vitte
│   │   ├── cli
│   │   │   ├── ansi.vitte
│   │   │   ├── prompt.vitte
│   │   ├── compress
│   │   │   ├── deflate.vitte
│   │   │   ├── lz4.vitte
│   │   │   ├── zlib.vitte
│   │   │   ├── zstd.vitte
│   │   ├── concurrent
│   │   │   ├── channel.vitte
│   │   │   ├── taskpool.vitte
│   │   │   ├── thread.vitte
│   │   ├── core
│   │   │   ├── arena.vitte
│   │   │   ├── bitset.vitte
│   │   │   ├── chrono.vitte
│   │   │   ├── iter.vitte
│   │   │   ├── map.vitte
│   │   │   ├── option.vitte
│   │   │   ├── rc.vitte
│   │   │   ├── result.vitte
│   │   │   ├── ring_buffer.vitte
│   │   │   ├── set.vitte
│   │   │   ├── stringx.vitte
│   │   │   ├── tuple.vitte
│   │   │   ├── vec.vitte
│   │   ├── crypto
│   │   │   ├── base64.vitte
│   │   │   ├── hash.vitte
│   │   │   ├── hkdf.vitte
│   │   │   ├── hmac.vitte
│   │   │   ├── pbkdf2.vitte
│   │   ├── db
│   │   │   ├── cache_lmdb.vitte
│   │   │   ├── kv.vitte
│   │   │   ├── postgres.vitte
│   │   │   ├── sqlite.vitte
│   │   ├── ds
│   │   │   ├── bloom.vitte
│   │   │   ├── btree.vitte
│   │   │   ├── deque.vitte
│   │   │   ├── linked_list.vitte
│   │   │   ├── priority_queue.vitte
│   │   │   ├── rope.vitte
│   │   ├── ffi
│   │   │   ├── c
│   │   │   │   ├── ffi_c.vitte
│   │   │   ├── cpp
│   │   │   │   ├── ffi_cpp.vitte
│   │   │   ├── tests
│   │   │   │   ├── ffi_test.vitte
│   │   │   ├── wasm
│   │   │   ├── ffi.vitte
│   │   ├── formats
│   │   │   ├── csv.vitte
│   │   │   ├── ini.vitte
│   │   │   ├── json.vitte
│   │   ├── i18n
│   │   │   ├── catalog.vitte
│   │   │   ├── datetime.vitte
│   │   │   ├── i18n.vitte
│   │   │   ├── loader_json.vitte
│   │   │   ├── locale.vitte
│   │   │   ├── message.vitte
│   │   │   ├── number.vitte
│   │   │   ├── plural.vitte
│   │   │   ├── README.md
│   │   │   ├── relative_time.vitte
│   │   ├── image
│   │   │   ├── jpeg.vitte
│   │   │   ├── png.vitte
│   │   ├── math
│   │   │   ├── decimal.vitte
│   │   │   ├── mathx.vitte
│   │   │   ├── rand.vitte
│   │   ├── net
│   │   │   ├── grpc.vitte
│   │   │   ├── http_client.vitte
│   │   │   ├── mqtt.vitte
│   │   │   ├── tcp.vitte
│   │   ├── security
│   │   │   ├── acl.vitte
│   │   │   ├── tls.vitte
│   │   ├── sys
│   │   │   ├── env.vitte
│   │   │   ├── fs.vitte
│   │   │   ├── path.vitte
│   │   │   ├── process.vitte
│   │   │   ├── time.vitte
│   │   ├── text
│   │   │   ├── slug.vitte
│   │   │   ├── tokenize.vitte
│   │   │   ├── unicode.vitte
│   │   ├── utils
│   │   │   ├── log.vitte
│   │   │   ├── uuid.vitte
│   │   │   ├── validate.vitte
│   │   ├── wasm
│   │   │   ├── component.vitte
│   │   │   ├── wasi.vitte
│   ├── tests
│   │   ├── fs_io.vitte
│   │   ├── http_smoke.vitte
│   │   ├── iter_map.vitte
│   │   ├── json_parse.vitte
│   │   ├── rand_uuid.vitte
│   ├── README.md
│   ├── vitte.toml
├── tests
│   ├── compiler
│   ├── fixtures
│   ├── integration
│   ├── performance
│   ├── unit
│   ├── vm
├── tools
│   ├── vitc
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitcc
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitpm
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitte-bench
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitte-doc
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitte-fmt
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitte-profile
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitx
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
│   ├── vitxx
│   │   ├── src
│   │   │   ├── main.rs
│   │   ├── Cargo.toml
├── xtask
│   ├── src
│   │   ├── main.rs
│   ├── Cargo.toml
├── .gitattributes
├── .gitignore
├── arborescence.md
├── build.rs
├── Cargo.lock
├── Cargo.toml
├── clippy.toml
├── CONTRIBUTING.md
├── deny.toml
├── LICENSE
├── main.vitte
├── Makefile
├── package.json
├── package-lock.json
├── README.md
├── rustfmt.toml
├── rust-toolchain.toml
├── tree.ps1
```
