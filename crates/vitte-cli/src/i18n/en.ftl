# English translations
status.ok = OK
status.info = INFO
status.warn = WARN

doctor.status.ok = [OK]
doctor.status.warn = [WARN]
doctor.status.error = [ERR]

doctor.suggestion.inline = suggestion: {text}
doctor.suggestions.title = Global suggestions:
doctor.suggestions.item = {index}. {text}

doctor.report.header = Vitte Doctor — workspace: {workspace} ({errors_label}, {warnings_label})

doctor.count.errors.zero = 0 errors
doctor.count.errors.one = 1 error
doctor.count.errors.other = {count} errors

doctor.count.warnings.zero = 0 warnings
doctor.count.warnings.one = 1 warning
doctor.count.warnings.other = {count} warnings

doctor.check.rustc.name = rustc toolchain
doctor.check.rustc.not_found = rustc not found ({error})
doctor.check.rustc.recommend.install = Install Rust via https://rustup.rs and ensure it is on PATH.
doctor.check.rustc.exec_failed = running `rustc --version` failed (code {code})
doctor.check.rustc.recommend.verify = Verify your Rust installation or reinstall rustup.
doctor.check.rustc.outdated = detected version {version} (minimum recommended {minimum})
doctor.check.rustc.recommend.update = Update Rust with `rustup update`.
doctor.check.rustc.ok = version ok ({version})
doctor.check.rustc.parse_error = unexpected version output: {output}

doctor.check.cargo.name = cargo binary
doctor.check.cargo.ok = {version}
doctor.check.cargo.exec_failed = `cargo --version` returned exit code {code}
doctor.check.cargo.recommend.verify = Verify your Rust installation (cargo is provided by rustup).
doctor.check.cargo.not_found = cargo not found ({error})
doctor.check.cargo.recommend.install = Install Rust via rustup to get cargo.

doctor.check.cache.name = target cache
doctor.check.cache.missing = target directory {path} not found
doctor.check.cache.recommend.build = Run `cargo build` to initialise the cache.
doctor.check.cache.write_fail = cannot write inside {path} ({error})
doctor.check.cache.recommend.perms = Check directory permissions or rebuild with appropriate privileges.
doctor.check.cache.ok = directory {path} is writable
doctor.check.cache.open_fail = cannot open {path} ({error})

doctor.check.modules.name = embedded modules
doctor.check.modules.ok = {path} present
doctor.check.modules.missing = {path} missing
doctor.check.modules.recommend.init = Initialise modules via `git submodule update --init --recursive`.

doctor.prompt.purge_cache = Purge cache directory {path}?

doctor.action.cache_purged = (target cache purged)
doctor.action.cache_kept = (cache kept)
doctor.action.cache_missing = (no target cache found)
