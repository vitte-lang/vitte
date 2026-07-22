package Vitte::City::Athens::Path;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    join_path normalize_path basename dirname extension without_extension
    replace_extension is_absolute is_relative split_path path_components
);

sub _as_slash_path {
    my ($path) = @_;
    $path = '' unless defined $path;
    $path =~ s{\\}{/}g;
    return $path;
}

sub join_path {
    my @parts = grep { defined $_ && length $_ } @_;
    return '' unless @parts;
    my $first = _as_slash_path($parts[0]);
    my $absolute = $first =~ m{^/} ? 1 : 0;
    my $drive = '';
    if ($first =~ s{^([A-Za-z]:)}{}) {
        $drive = $1;
    }
    my @clean;
    for my $part (@parts) {
        $part = _as_slash_path($part);
        $part =~ s{^[A-Za-z]:}{};
        push @clean, grep { length $_ } split m{/+}, $part;
    }
    my $joined = join('/', @clean);
    $joined = "/$joined" if $absolute;
    $joined = "$drive/$joined" if length $drive && $joined !~ m{^/};
    $joined = "$drive$joined" if length $drive && $joined =~ m{^/};
    return normalize_path($joined);
}

sub normalize_path {
    my ($path) = @_;
    $path = _as_slash_path($path);
    return '.' if $path eq '';
    my $drive = '';
    if ($path =~ s{^([A-Za-z]:)}{}) {
        $drive = $1;
    }
    my $absolute = $path =~ m{^/} ? 1 : 0;
    my @out;
    for my $part (split m{/+}, $path) {
        next if $part eq '' || $part eq '.';
        if ($part eq '..') {
            if (@out && $out[-1] ne '..') {
                pop @out;
            } elsif (!$absolute) {
                push @out, $part;
            }
            next;
        }
        push @out, $part;
    }
    my $result = join('/', @out);
    $result = "/$result" if $absolute;
    $result = "$drive$result" if length $drive;
    return length($result) ? $result : ($absolute ? '/' : '.');
}

sub basename {
    my ($path) = @_;
    $path = normalize_path($path);
    $path =~ s{/$}{};
    $path =~ s{.*/}{};
    return $path;
}

sub dirname {
    my ($path) = @_;
    $path = normalize_path($path);
    return '/' if $path eq '/';
    $path =~ s{/[^/]*$}{};
    return length($path) ? $path : '.';
}

sub extension {
    my ($path) = @_;
    my $base = basename($path);
    return $base =~ /\.([^.]+)$/ ? $1 : '';
}

sub without_extension {
    my ($path) = @_;
    $path =~ s{\.[^./\\]+$}{};
    return $path;
}

sub replace_extension {
    my ($path, $ext) = @_;
    $ext = '' unless defined $ext;
    $ext =~ s/^\.//;
    my $stem = without_extension($path);
    return length($ext) ? "$stem.$ext" : $stem;
}

sub is_absolute {
    my ($path) = @_;
    $path = _as_slash_path($path);
    return $path =~ m{^/} || $path =~ m{^[A-Za-z]:/} ? 1 : 0;
}

sub is_relative {
    my ($path) = @_;
    return is_absolute($path) ? 0 : 1;
}

sub split_path {
    my ($path) = @_;
    $path = normalize_path($path);
    return [] if $path eq '/';
    $path =~ s{^[A-Za-z]:/?}{};
    $path =~ s{^/}{};
    return [ grep { length $_ } split m{/+}, $path ];
}

sub path_components {
    return split_path($_[0]);
}

1;
