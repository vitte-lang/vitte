# Cairo API

Cairo provides a broad, versioned structured-data utility surface for Vitte package tooling. The public API is exported by `Vitte::City::Cairo` and implemented by focused submodules.

## Scalars

Escape, decode, type, convert, trim, normalize case, default, and compare scalar values.

- `encode_scalar`
- `decode_scalar`
- `escape_scalar`
- `unescape_scalar`
- `scalar_type`
- `scalar_is_null`
- `scalar_is_bool`
- `scalar_is_number`
- `scalar_is_integer`
- `scalar_to_bool`
- `scalar_to_number`
- `scalar_to_string`
- `scalar_trim`
- `scalar_lower`
- `scalar_upper`
- `scalar_default`
- `scalar_compare`

## Records

Encode, decode, mutate, project, omit, validate required keys, merge, rename, diff, and render query records.

- `encode_record`
- `decode_record`
- `record`
- `record_get`
- `record_set`
- `record_has`
- `record_delete`
- `record_keys`
- `record_values`
- `record_pairs`
- `record_merge`
- `record_project`
- `record_omit`
- `record_require`
- `record_apply_defaults`
- `record_rename`
- `record_diff`
- `record_to_query`

## Lists

Encode, decode, mutate, slice, sort, reverse, join, search, compact, flatten, and deduplicate lists.

- `encode_list`
- `decode_list`
- `list`
- `list_push`
- `list_pop`
- `list_shift`
- `list_unshift`
- `list_unique`
- `list_sort`
- `list_reverse`
- `list_slice`
- `list_join`
- `list_contains`
- `list_index_of`
- `list_count`
- `list_compact`
- `list_flatten`

## Tables

Create tables from records, add rows, inspect columns, filter, sort, limit, offset, group, index, and export rows.

- `table`
- `table_columns`
- `table_rows`
- `table_add_row`
- `table_count`
- `table_project`
- `table_where`
- `table_sort`
- `table_limit`
- `table_offset`
- `table_group_count`
- `table_find`
- `table_first`
- `table_column_values`
- `table_to_records`
- `table_from_records`
- `table_join_text`
- `table_index_by`

## Schemas

Define fields and schemas, inspect required/optional/default fields, apply defaults, and validate records/tables.

- `schema`
- `field`
- `schema_fields`
- `schema_required`
- `schema_optional`
- `schema_defaults`
- `schema_apply_defaults`
- `schema_validate_record`
- `schema_validate_table`
- `schema_errors`
- `schema_is_valid`
- `type_string`
- `type_number`
- `type_integer`
- `type_bool`
- `type_list`

## Codecs

Encode/decode documents, lines, pairs, tables, canonical JSON, document mutation, and stable hashes.

- `encode_document`
- `decode_document`
- `encode_lines`
- `decode_lines`
- `encode_pairs`
- `decode_pairs`
- `encode_table`
- `decode_table`
- `document_get`
- `document_set`
- `document_delete`
- `document_keys`
- `canonical_json`
- `stable_hash`

## Structured Errors

Create stable Cairo errors with code, message, context, path, field, hash rendering, string rendering, and code matching.

- `cairo_error`
- `error_code`
- `error_message`
- `error_context`
- `error_path`
- `error_field`
- `error_to_hash`
- `error_to_string`
- `error_with_context`
- `error_is`

## Distribution Contract

- `META.json` declares every module and core runtime dependency.
- `packages.json` mirrors `@EXPORT_OK` exactly.
- `MANIFEST` lists every package file required by the reproducible archive gate.
- `t/basic.t` mentions every exported symbol and covers all public groups.
