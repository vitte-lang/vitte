# vitte-perl-athens

Version: `0.1.0`

Athens is the Vitte Perl package for portable project filesystem work: temporary resources, cleanup transactions, path normalization, manifests, permission probes, atomic file IO, and structured package errors.

## Module

`Vitte::City::Athens`

## Public API

### Temporary Resources

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

### Cleanup Plans

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

### Path Operations

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

### Permissions

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

### Atomic IO

Read and write text or bytes through temporary files, flush, optional fsync, chmod preservation, and atomic rename.

- `atomic_write`
- `atomic_write_bytes`
- `read_text`
- `read_bytes`
- `write_text`
- `write_bytes`

### Manifest

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

### Structured Errors

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

## Submodules

- `Vitte::City::Athens::Path`
- `Vitte::City::Athens::Temp`
- `Vitte::City::Athens::Cleanup`
- `Vitte::City::Athens::Manifest`
- `Vitte::City::Athens::Permissions`
- `Vitte::City::Athens::Atomic`
- `Vitte::City::Athens::Error`

## Guarantees

- Versioned distribution metadata stays fixed at `0.1.0`.
- Every public export is exercised by `t/basic.t`.
- Every module compiles independently under `PERL5LIB`.
- The repository gate builds a reproducible archive, writes SHA256 checksums, extracts it, and reruns install tests.
- Cleanup is explicit and previewable before destructive operations.
- Atomic writes use temporary files in the destination directory and preserve existing modes unless a mode is supplied.

## Checks

```sh
perl -Ilib t/00-load.t
perl -Ilib t/basic.t
perl -Ilib t/path.t
perl -Ilib t/temp.t
perl -Ilib t/cleanup.t
perl -Ilib t/manifest.t
perl -Ilib t/permissions.t
perl -Ilib t/atomic.t
perl -Ilib t/errors.t
perl -Ilib t/install-perl5lib.t
perl -Ilib t/regression.t
```
