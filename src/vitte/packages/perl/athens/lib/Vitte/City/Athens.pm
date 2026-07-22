package Vitte::City::Athens;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Athens::Atomic qw(atomic_write read_text write_text);
use Vitte::City::Athens::Cleanup qw(cleanup_add cleanup_dry_run cleanup_plan cleanup_run);
use Vitte::City::Athens::Error qw(athens_error);
use Vitte::City::Athens::Manifest qw(manifest manifest_add_dir manifest_add_file manifest_paths manifest_size);
use Vitte::City::Athens::Path qw(
    basename dirname extension is_absolute is_relative join_path normalize_path
    path_components replace_extension split_path without_extension
);
use Vitte::City::Athens::Permissions qw(ensure_dir is_readable is_writable permissions);
use Vitte::City::Athens::Temp qw(temp_dir temp_file temp_name temp_path);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    temp_name temp_path temp_file temp_dir
    cleanup_plan cleanup_add cleanup_run cleanup_dry_run
    join_path normalize_path basename dirname extension without_extension
    replace_extension is_absolute is_relative split_path path_components
    ensure_dir atomic_write read_text write_text
    manifest manifest_add_file manifest_add_dir manifest_paths manifest_size
    permissions is_readable is_writable athens_error
);

1;
