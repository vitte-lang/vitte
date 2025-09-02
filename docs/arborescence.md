```
├── .github
│   ├── wotkflows
│   │   ├── ci.yml
│   │   ├── release.yml
├── assets
├── benchmarks
│   ├── benches
│   │   ├── e2e_pipeline.rs
│   │   ├── end_to_end.rs
│   │   ├── ir_passes.rs
│   │   ├── lexer.rs
│   │   ├── parser.rs
│   │   ├── vm_hotpath.rs
│   │   ├── vm_micro.rs
│   ├── data
│   │   ├── bytecode
│   │   ├── projects
│   ├── scripts
│   │   ├── prepare.sh
│   ├── Cargo.toml
├── crates
│   ├── vitte-ast
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-cli
│   │   ├── src
│   │   │   ├── lib.rs
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
│   ├── vitte-stdlib
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
│   │   ├── build.rs
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
├── docs
│   ├── ADR
│   │   ├── 000-template.md
│   │   ├── 001-workspace-layout.md
│   ├── ARCHITECTURE
│   │   ├── overview.md
│   │   ├── pipeline.md
│   ├── CLI
│   │   ├── guide.md
│   ├── IR
│   │   ├── ir.md
│   ├── LSP
│   │   ├── guide.md
│   ├── RUNTIME
│   │   ├── overview.md
│   ├── STDLIB
│   │   ├── guide.md
│   ├── VITBC
│   │   ├── spec.md
│   ├── VM
│   │   ├── overview.md
│   ├── arborescence.md
│   ├── BENCHMARKS.md
│   ├── BUILD.md
│   ├── CI.md
│   ├── COMPLETIONS.md
│   ├── GETTING_STARTED.md
│   ├── README.md
│   ├── RELEASE.md
│   ├── SECURITY.md
│   ├── STYLE.md
│   ├── TESTING_AND_QA.md
│   ├── TROUBLESHOOTING.md
│   ├── WASM.md
├── examples
│   ├── modules_vitte
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
├── modules
│   ├── src
│   │   ├── lib.rs
│   ├── vitte-cs2d
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-fmt
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-gfx2d
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-hal
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-mm
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-opt
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-resolver
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-sched
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-sys
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── vitte-typer
│   │   ├── src
│   │   │   ├── lib.rs
│   │   ├── Cargo.toml
│   ├── Cargo.toml
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
│   ├── bench.ps1
│   ├── bench.sh
│   ├── check.ps1
│   ├── check.sh
│   ├── completions.ps1
│   ├── completions.sh
│   ├── docs.ps1
│   ├── docs.sh
│   ├── lsp.ps1
│   ├── lsp.sh
│   ├── README.md
│   ├── release-tag.ps1
│   ├── release-tag.sh
│   ├── wasm-build.ps1
│   ├── wasm-build.sh
├── security
│   ├── policy.md
│   ├── threat-model.md
├── std
│   ├── src
│   │   ├── lib.rs
│   ├── Cargo.toml
│   ├── README.md
├── tests
│   ├── src
│   │   ├── lib.rs
│   ├── tests
│   │   ├── lexer_smoke.rs
│   │   ├── parser_roundtrip.rs
│   ├── Cargo.toml
│   ├── README.md
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
├── package.json
├── package-lock.json
├── README.md
├── rustfmt.toml
├── rust-toolchain.toml
├── tree.ps1
```
