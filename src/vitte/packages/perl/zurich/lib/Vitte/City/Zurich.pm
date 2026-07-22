package Vitte::City::Zurich;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(ordered_map map_put map_get map_keys set set_has set_add group_by index_by);

sub ordered_map { return { order => [], values => {} } }

sub map_put {
    my ($map, $key, $value) = @_;
    push @{ $map->{order} }, $key unless exists $map->{values}->{$key};
    $map->{values}->{$key} = $value;
    return $map;
}

sub map_get { return $_[0]->{values}->{$_[1]} }
sub map_keys { return @{ $_[0]->{order} || [] } }

sub set {
    my @values = @_;
    my %seen = map { $_ => 1 } @values;
    return \%seen;
}

sub set_has { return exists $_[0]->{$_[1]} ? 1 : 0 }
sub set_add { $_[0]->{$_[1]} = 1; return $_[0] }

sub group_by {
    my ($items, $fn) = @_;
    my %groups;
    push @{ $groups{ $fn->($_) } }, $_ for @$items;
    return \%groups;
}

sub index_by {
    my ($items, $fn) = @_;
    my %index;
    $index{ $fn->($_) } = $_ for @$items;
    return \%index;
}

1;
