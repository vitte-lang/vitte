package Vitte::City::Athens::Temp;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Athens::Atomic qw(write_text);
use Vitte::City::Athens::Cleanup qw(cleanup_add_dir cleanup_add_file cleanup_plan);
use Vitte::City::Athens::Path qw(join_path);
use Vitte::City::Athens::Permissions qw(ensure_dir);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    temp_name temp_path temp_file temp_dir temp_scope temp_counter
    temp_reset_counter temp_file_pair temp_dir_plan
);

my $COUNTER = 0;

sub temp_name {
    my ($prefix, $suffix) = @_;
    $prefix = 'vitte' unless defined $prefix && length $prefix;
    $suffix = '' unless defined $suffix;
    $COUNTER++;
    my $stamp = time() . '-' . $$ . '-' . $COUNTER;
    return $prefix . '-' . $stamp . $suffix;
}

sub temp_path {
    my ($dir, $prefix, $suffix) = @_;
    $dir = $ENV{TMPDIR} || '/tmp' unless defined $dir && length $dir;
    return join_path($dir, temp_name($prefix, $suffix));
}

sub temp_dir {
    my ($dir, $prefix) = @_;
    my $path = temp_path($dir, $prefix || 'vitte-dir', '');
    ensure_dir($path);
    return $path;
}

sub temp_file {
    my ($dir, $prefix, $suffix, $content) = @_;
    my $path = temp_path($dir, $prefix || 'vitte-file', defined $suffix ? $suffix : '.tmp');
    write_text($path, defined $content ? $content : '');
    return $path;
}

sub temp_counter {
    return $COUNTER;
}

sub temp_reset_counter {
    $COUNTER = 0;
    return $COUNTER;
}

sub temp_scope {
    my ($dir, $prefix) = @_;
    my $root = temp_dir($dir, $prefix || 'vitte-scope');
    my $plan = cleanup_plan();
    cleanup_add_dir($plan, $root, { recursive => 1, ignore_missing => 1 });
    return {
        root => $root,
        cleanup => $plan,
    };
}

sub temp_file_pair {
    my ($dir, $prefix) = @_;
    my $left = temp_file($dir, ($prefix || 'vitte-pair') . '-left', '.tmp', '');
    my $right = temp_file($dir, ($prefix || 'vitte-pair') . '-right', '.tmp', '');
    return ($left, $right);
}

sub temp_dir_plan {
    my ($dir, $prefix) = @_;
    my $root = temp_dir($dir, $prefix || 'vitte-plan');
    my $file = temp_file($root, 'payload', '.tmp', '');
    my $plan = cleanup_plan();
    cleanup_add_file($plan, $file, { ignore_missing => 1 });
    cleanup_add_dir($plan, $root, { recursive => 1, ignore_missing => 1 });
    return {
        root => $root,
        file => $file,
        cleanup => $plan,
    };
}

1;
