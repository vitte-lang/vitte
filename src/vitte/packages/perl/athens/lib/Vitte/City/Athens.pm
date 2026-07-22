package Vitte::City::Athens;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(temp_name temp_path cleanup_plan join_path basename dirname extension);

sub temp_name {
    my ($prefix, $suffix) = @_;
    $prefix = 'vitte' unless defined $prefix;
    $suffix = '' unless defined $suffix;
    my $stamp = time() . '-' . $$ . '-' . int(rand(1_000_000));
    return $prefix . '-' . $stamp . $suffix;
}

sub join_path {
    my @parts = grep { defined $_ && length $_ } @_;
    my $path = join('/', @parts);
    $path =~ s{/+}{/}g;
    return $path;
}

sub temp_path {
    my ($dir, $prefix, $suffix) = @_;
    $dir = '/tmp' unless defined $dir && length $dir;
    return join_path($dir, temp_name($prefix, $suffix));
}

sub cleanup_plan {
    my @paths = @_;
    return { remove => [ @paths ], count => scalar @paths };
}

sub basename { my ($p) = @_; $p =~ s{.*/}{}; return $p }
sub dirname { my ($p) = @_; $p =~ s{/[^/]*$}{}; return length($p) ? $p : '.' }
sub extension { my ($p) = @_; return $p =~ /\.([^.\/]+)$/ ? $1 : '' }

1;
