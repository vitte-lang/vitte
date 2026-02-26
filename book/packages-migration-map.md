# Packages Migration Map (Auto)

Generated from legacy allowlists in `tools/`.

| Location | Old import | New import |
|---|---|---|
| tests/diag_snapshots/resolve/legacy_import_path_deprecated_fail_on_warning.vit:1 | `vitte/abi/abi` | `vitte/abi` |
| tests/diag_snapshots/resolve/legacy_import_path_deprecated_warn.vit:1 | `vitte/abi/abi` | `vitte/abi` |
| tests/modules/mod_doctor_legacy/main.vit:1 | `vitte/abi/abi` | `vitte/abi` |
| tests/modules/mod_doctor_write_cases/pull_legacy.vit:1 | `vitte/http/http` | `vitte/http` |
| tests/modules/mod_doctor_write_cases/use_legacy.vit:1 | `vitte/abi/abi` | `vitte/abi` |

## Legacy Wrappers (`<pkg>.vit` -> `mod.vit`)

| Package | Old entry | New entry |
|---|---|---|
| `abi` | `abi/abi.vit` | `abi/mod.vit` |
| `actor` | `actor/actor.vit` | `actor/mod.vit` |
| `alerts` | `alerts/alerts.vit` | `alerts/mod.vit` |
| `alloc` | `alloc/alloc.vit` | `alloc/mod.vit` |
| `array` | `array/array.vit` | `array/mod.vit` |
| `ast` | `ast/ast.vit` | `ast/mod.vit` |
| `async` | `async/async.vit` | `async/mod.vit` |
| `audio` | `audio/audio.vit` | `audio/mod.vit` |

## Legacy Names (`_` -> `/`)

| Legacy name | Target hierarchy |
|---|---|
| `circuit_breaker` | `circuit/breaker` |
| `config_derive` | `config/derive` |
| `const_eval` | `const/eval` |
| `feature_flags` | `feature/flags` |
| `http_client` | `http/client` |
| `mock_http` | `mock/http` |
| `random_secure` | `random/secure` |
| `rate_limit` | `rate/limit` |
| `schema_registry` | `schema/registry` |
