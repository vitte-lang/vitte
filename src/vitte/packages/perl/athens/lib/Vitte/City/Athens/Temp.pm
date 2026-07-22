package Vitte::City::Athens::Temp;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Athens::Atomic qw(write_text);
use Vitte::City::Athens::Path qw(join_path);
use Vitte::City::Athens::Permissions qw(ensure_dir);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(temp_name temp_path temp_file temp_dir);

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

1;
