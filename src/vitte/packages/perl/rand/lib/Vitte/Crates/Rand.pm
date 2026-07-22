package Vitte::Crates::Rand;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(seeded next_u32 range choice);

sub seeded { my ($seed)=@_; return { state => ($seed || 1) & 0xffffffff }; }
sub next_u32 { my ($rng)=@_; $rng->{state} = (1664525 * $rng->{state} + 1013904223) & 0xffffffff; return $rng->{state}; }
sub range { my ($rng,$min,$max)=@_; return $min if $max <= $min; return $min + (next_u32($rng) % ($max - $min)); }
sub choice { my ($rng,$items)=@_; return undef unless @$items; return $items->[range($rng,0,scalar @$items)]; }

1;
