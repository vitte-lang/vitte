package Vitte::City::Quito;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(seeded next_u32 range choice shuffle bytes_random);

sub seeded {
    my ($seed) = @_;
    return { state => int($seed || 1) & 0xffffffff };
}

sub next_u32 {
    my ($rng) = @_;
    $rng->{state} = (1664525 * $rng->{state} + 1013904223) & 0xffffffff;
    return $rng->{state};
}

sub range {
    my ($rng, $min, $max) = @_;
    return $min + (next_u32($rng) % (($max - $min) + 1));
}

sub choice {
    my ($rng, $items) = @_;
    return undef unless @$items;
    return $items->[range($rng, 0, @$items - 1)];
}

sub shuffle {
    my ($rng, $items) = @_;
    my @out = @$items;
    for (my $i = @out - 1; $i > 0; $i--) {
        my $j = range($rng, 0, $i);
        @out[$i, $j] = @out[$j, $i];
    }
    return \@out;
}

sub bytes_random {
    my ($rng, $count) = @_;
    return [ map { range($rng, 0, 255) } 1 .. $count ];
}

1;
