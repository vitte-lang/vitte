package Vitte::City::Athens;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Athens::Atomic qw(atomic_write atomic_write_bytes read_bytes read_text write_bytes write_text);
use Vitte::City::Athens::Cleanup qw(
    cleanup_add cleanup_add_callback cleanup_add_dir cleanup_add_file cleanup_clear
    cleanup_dry_run cleanup_errors cleanup_is_empty cleanup_plan cleanup_run cleanup_size
);
use Vitte::City::Athens::Error qw(
    athens_error error_code error_context error_is error_message error_operation
    error_path error_to_hash error_to_string error_with_context
);
use Vitte::City::Athens::Manifest qw(
    manifest manifest_add_dir manifest_add_entry manifest_add_file manifest_count
    manifest_find manifest_paths manifest_remove manifest_size manifest_sort
    manifest_to_hash manifest_to_text
);
use Vitte::City::Athens::Path qw(
    basename dirname extension is_absolute is_relative join_path normalize_path
    path_components replace_extension split_path without_extension path_stem
    path_depth path_is_root path_has_extension relative_path resolve_path
    common_prefix change_basename
);
use Vitte::City::Athens::Permissions qw(
    chmod_path ensure_dir file_group_id file_mode_summary file_owner_id
    is_executable is_readable is_writable permission_bits permissions
);
use Vitte::City::Athens::Temp qw(
    temp_counter temp_dir temp_dir_plan temp_file temp_file_pair temp_name
    temp_path temp_reset_counter temp_scope
);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    temp_name temp_path temp_file temp_dir
    temp_scope temp_counter temp_reset_counter temp_file_pair temp_dir_plan
    cleanup_plan cleanup_add cleanup_add_file cleanup_add_dir cleanup_add_callback
    cleanup_run cleanup_dry_run cleanup_clear cleanup_size cleanup_is_empty cleanup_errors
    join_path normalize_path basename dirname extension without_extension
    replace_extension is_absolute is_relative split_path path_components
    path_stem path_depth path_is_root path_has_extension relative_path resolve_path
    common_prefix change_basename
    ensure_dir permissions is_readable is_writable is_executable permission_bits
    chmod_path file_owner_id file_group_id file_mode_summary
    atomic_write atomic_write_bytes read_text read_bytes write_text write_bytes
    manifest manifest_add_file manifest_add_dir manifest_add_entry manifest_paths
    manifest_size manifest_count manifest_find manifest_remove manifest_sort
    manifest_to_text manifest_to_hash
    athens_error error_code error_message error_context error_path error_operation
    error_with_context error_to_string error_to_hash error_is
);

1;
