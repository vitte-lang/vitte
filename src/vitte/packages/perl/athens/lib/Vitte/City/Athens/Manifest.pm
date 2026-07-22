package Vitte::City::Athens::Manifest;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    manifest manifest_add_file manifest_add_dir manifest_paths manifest_size
    manifest_count manifest_find manifest_remove manifest_sort manifest_to_text
    manifest_to_hash manifest_add_entry
);

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

sub manifest_add_entry {
    my ($manifest, %entry) = @_;
    $entry{type} ||= 'file';
    $entry{path} = '' unless defined $entry{path};
    $entry{size} = 0 unless defined $entry{size};
    $entry{mode} ||= $entry{type} eq 'dir' ? '0755' : '0644';
    push @{ $manifest->{entries} }, { %entry };
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

sub manifest_count {
    my ($manifest) = @_;
    return scalar @{ $manifest->{entries} || [] };
}

sub manifest_find {
    my ($manifest, $path) = @_;
    for my $entry (@{ $manifest->{entries} || [] }) {
        return { %{$entry} } if $entry->{path} eq $path;
    }
    return undef;
}

sub manifest_remove {
    my ($manifest, $path) = @_;
    my @kept = grep { $_->{path} ne $path } @{ $manifest->{entries} || [] };
    my $removed = scalar(@{ $manifest->{entries} || [] }) - scalar(@kept);
    $manifest->{entries} = \@kept;
    return $removed;
}

sub manifest_sort {
    my ($manifest) = @_;
    $manifest->{entries} = [
        sort { $a->{path} cmp $b->{path} || $a->{type} cmp $b->{type} }
        @{ $manifest->{entries} || [] }
    ];
    return $manifest;
}

sub manifest_to_text {
    my ($manifest) = @_;
    return join('', map {
        join(' ', $_->{type}, $_->{mode}, $_->{size}, $_->{path}) . "\n"
    } @{ $manifest->{entries} || [] });
}

sub manifest_to_hash {
    my ($manifest) = @_;
    return {
        entries => [
            map { { %{$_} } } @{ $manifest->{entries} || [] }
        ],
        count => manifest_count($manifest),
        size => manifest_size($manifest),
    };
}

1;
