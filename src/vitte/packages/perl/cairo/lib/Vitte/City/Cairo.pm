package Vitte::City::Cairo;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Cairo::Codec qw(
    canonical_json decode_document decode_lines decode_pairs decode_table document_delete
    document_get document_keys document_set encode_document encode_lines encode_pairs
    encode_table stable_hash
);
use Vitte::City::Cairo::Error qw(
    cairo_error error_code error_context error_field error_is error_message error_path
    error_to_hash error_to_string error_with_context
);
use Vitte::City::Cairo::List qw(
    decode_list encode_list list list_compact list_contains list_count list_flatten
    list_index_of list_join list_pop list_push list_reverse list_shift list_slice
    list_sort list_unique list_unshift
);
use Vitte::City::Cairo::Record qw(
    decode_record encode_record record record_apply_defaults record_delete record_diff
    record_get record_has record_keys record_merge record_omit record_pairs record_project
    record_rename record_require record_set record_to_query record_values
);
use Vitte::City::Cairo::Scalar qw(
    decode_scalar encode_scalar escape_scalar scalar_compare scalar_default scalar_is_bool
    scalar_is_integer scalar_is_null scalar_is_number scalar_lower scalar_to_bool
    scalar_to_number scalar_to_string scalar_trim scalar_type scalar_upper unescape_scalar
);
use Vitte::City::Cairo::Schema qw(
    field schema schema_apply_defaults schema_defaults schema_errors schema_fields
    schema_is_valid schema_optional schema_required schema_validate_record
    schema_validate_table type_bool type_integer type_list type_number type_string
);
use Vitte::City::Cairo::Table qw(
    table table_add_row table_column_values table_columns table_count table_find
    table_first table_from_records table_group_count table_index_by table_join_text
    table_limit table_offset table_project table_rows table_sort table_to_records
    table_where
);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    encode_scalar
    decode_scalar
    escape_scalar
    unescape_scalar
    scalar_type
    scalar_is_null
    scalar_is_bool
    scalar_is_number
    scalar_is_integer
    scalar_to_bool
    scalar_to_number
    scalar_to_string
    scalar_trim
    scalar_lower
    scalar_upper
    scalar_default
    scalar_compare
    encode_record
    decode_record
    record
    record_get
    record_set
    record_has
    record_delete
    record_keys
    record_values
    record_pairs
    record_merge
    record_project
    record_omit
    record_require
    record_apply_defaults
    record_rename
    record_diff
    record_to_query
    encode_list
    decode_list
    list
    list_push
    list_pop
    list_shift
    list_unshift
    list_unique
    list_sort
    list_reverse
    list_slice
    list_join
    list_contains
    list_index_of
    list_count
    list_compact
    list_flatten
    table
    table_columns
    table_rows
    table_add_row
    table_count
    table_project
    table_where
    table_sort
    table_limit
    table_offset
    table_group_count
    table_find
    table_first
    table_column_values
    table_to_records
    table_from_records
    table_join_text
    table_index_by
    schema
    field
    schema_fields
    schema_required
    schema_optional
    schema_defaults
    schema_apply_defaults
    schema_validate_record
    schema_validate_table
    schema_errors
    schema_is_valid
    type_string
    type_number
    type_integer
    type_bool
    type_list
    encode_document
    decode_document
    encode_lines
    decode_lines
    encode_pairs
    decode_pairs
    encode_table
    decode_table
    document_get
    document_set
    document_delete
    document_keys
    canonical_json
    stable_hash
    cairo_error
    error_code
    error_message
    error_context
    error_path
    error_field
    error_to_hash
    error_to_string
    error_with_context
    error_is
);

1;
