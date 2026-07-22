package Vitte::City::Athens::Manifest;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(manifest manifest_add_file manifest_add_dir manifest_paths manifest_size);

sub manifest {
    return { entries => [] };
}

sub manifest_add_file {
    my ($manifest, $path, $size, $mode) = @_;
    push @{ $manifest->{entries} }, {
        type => 'file',
        path => $path,
        size => defined $size ? int($size) : 0,
        mode => defined $mode ? $mode : '0644',
    };
    return $manifest;
}

sub manifest_add_dir {
    my ($manifest, $path, $mode) = @_;
    push @{ $manifest->{entries} }, {
        type => 'dir',
        path => $path,
        size => 0,
        mode => defined $mode ? $mode : '0755',
    };
    return $manifest;
}

sub manifest_paths {
    my ($manifest) = @_;
    return [ map { $_->{path} } @{ $manifest->{entries} || [] } ];
}

sub manifest_size {
    my ($manifest) = @_;
    my $size = 0;
    $size += $_->{size} for @{ $manifest->{entries} || [] };
    return $size;
}

1;
