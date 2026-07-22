package Vitte::City::Lisbon;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(parse_version format_version compare_version is_prerelease satisfies_min bump_major bump_minor bump_patch);

sub parse_version {
    my ($text) = @_;
    die "invalid version: $text" unless $text =~ /^(\d+)\.(\d+)\.(\d+)(?:-([A-Za-z0-9_.-]+))?$/;
    return { major => int($1), minor => int($2), patch => int($3), prerelease => $4 };
}

sub format_version {
    my ($v) = @_;
    return join('.', $v->{major}, $v->{minor}, $v->{patch}) . (defined $v->{prerelease} ? '-' . $v->{prerelease} : '');
}

sub compare_version {
    my ($a, $b) = @_;
    $a = parse_version($a) unless ref $a;
    $b = parse_version($b) unless ref $b;
    for my $k (qw(major minor patch)) {
        return $a->{$k} <=> $b->{$k} if $a->{$k} != $b->{$k};
    }
    return 0 if !defined $a->{prerelease} && !defined $b->{prerelease};
    return 1 if !defined $a->{prerelease};
    return -1 if !defined $b->{prerelease};
    return $a->{prerelease} cmp $b->{prerelease};
}

sub is_prerelease { return defined parse_version($_[0])->{prerelease} ? 1 : 0 }
sub satisfies_min { return compare_version($_[0], $_[1]) >= 0 ? 1 : 0 }
sub bump_major { my $v = parse_version($_[0]); return ($v->{major} + 1) . '.0.0' }
sub bump_minor { my $v = parse_version($_[0]); return $v->{major} . '.' . ($v->{minor} + 1) . '.0' }
sub bump_patch { my $v = parse_version($_[0]); return $v->{major} . '.' . $v->{minor} . '.' . ($v->{patch} + 1) }

1;
