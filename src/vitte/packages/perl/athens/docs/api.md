# Athens API

Athens provides a broad, versioned filesystem utility surface for Vitte package tooling. The public API is exported by `Vitte::City::Athens` and implemented by focused submodules.

## Temporary Resources

Create deterministic temporary names, files, directories, scoped workspaces, file pairs, cleanup-backed plans, and resettable counters.

- `temp_name`
- `temp_path`
- `temp_file`
- `temp_dir`
- `temp_scope`
- `temp_counter`
- `temp_reset_counter`
- `temp_file_pair`
- `temp_dir_plan`

## Cleanup Plans

Build transactional cleanup plans for files, directories, callbacks, dry-runs, retained entries, errors, and explicit order.

- `cleanup_plan`
- `cleanup_add`
- `cleanup_add_file`
- `cleanup_add_dir`
- `cleanup_add_callback`
- `cleanup_run`
- `cleanup_dry_run`
- `cleanup_clear`
- `cleanup_size`
- `cleanup_is_empty`
- `cleanup_errors`

## Path Operations

Normalize, join, split, classify, compare, resolve, relativize, and rewrite portable path strings.

- `join_path`
- `normalize_path`
- `basename`
- `dirname`
- `extension`
- `without_extension`
- `replace_extension`
- `is_absolute`
- `is_relative`
- `split_path`
- `path_components`
- `path_stem`
- `path_depth`
- `path_is_root`
- `path_has_extension`
- `relative_path`
- `resolve_path`
- `common_prefix`
- `change_basename`

## Permissions

Create directories, inspect modes, readability, writability, executability, ownership ids, and chmod results.

- `ensure_dir`
- `permissions`
- `is_readable`
- `is_writable`
- `is_executable`
- `permission_bits`
- `chmod_path`
- `file_owner_id`
- `file_group_id`
- `file_mode_summary`

## Atomic IO

Read and write text or bytes through temporary files, flush, optional fsync, chmod preservation, and atomic rename.

- `atomic_write`
- `atomic_write_bytes`
- `read_text`
- `read_bytes`
- `write_text`
- `write_bytes`

## Manifest

Create package manifests, add typed entries, list paths, total sizes, count, find, remove, sort, and render text/hash views.

- `manifest`
- `manifest_add_file`
- `manifest_add_dir`
- `manifest_add_entry`
- `manifest_paths`
- `manifest_size`
- `manifest_count`
- `manifest_find`
- `manifest_remove`
- `manifest_sort`
- `manifest_to_text`

## Structured Errors

Create stable Athens errors with code, message, path, operation, context, hash rendering, string rendering, and code matching.

- `manifest_to_hash`
- `athens_error`
- `error_code`
- `error_message`
- `error_context`
- `error_path`
- `error_operation`
- `error_with_context`
- `error_to_string`
- `error_to_hash`
- `error_is`

## Distribution Contract

- `META.json` declares every module and core runtime dependency.
- `packages.json` mirrors `@EXPORT_OK` exactly.
- `MANIFEST` lists every package file required by the reproducible archive gate.
- `t/basic.t` mentions every exported symbol, and focused tests cover each submodule.
