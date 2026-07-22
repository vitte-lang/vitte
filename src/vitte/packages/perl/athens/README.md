# vitte-perl-athens

Version: `0.1.0`

Athens is a Perl utility package for temporary paths, cleanup plans, path
normalization, file manifests, permission probes, atomic text IO, and structured
package errors.

## Module

`Vitte::City::Athens`

## Public API

- `temp_name`
- `temp_path`
- `temp_file`
- `temp_dir`
- `cleanup_plan`
- `cleanup_add`
- `cleanup_run`
- `cleanup_dry_run`
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
- `ensure_dir`
- `atomic_write`
- `read_text`
- `write_text`
- `manifest`
- `manifest_add_file`
- `manifest_add_dir`
- `manifest_paths`
- `manifest_size`
- `permissions`
- `is_readable`
- `is_writable`
- `athens_error`

## Submodules

- `Vitte::City::Athens::Path`
- `Vitte::City::Athens::Temp`
- `Vitte::City::Athens::Cleanup`
- `Vitte::City::Athens::Manifest`
- `Vitte::City::Athens::Permissions`
- `Vitte::City::Athens::Atomic`
- `Vitte::City::Athens::Error`

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

The repository gate also compiles every module, compiles examples, verifies the
machine index, builds a reproducible archive, writes a SHA256 checksum, extracts
the archive, and reruns the install test through `PERL5LIB`.
