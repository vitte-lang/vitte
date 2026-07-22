package Vitte::Crates::Rayon;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(join par_map par_sum);

sub join { my ($left,$right)=@_; return ($left->(), $right->()); }
sub par_map { my ($items,$f)=@_; return [ map { $f->($_) } @$items ]; }
sub par_sum { my ($items)=@_; my $s=0; $s += $_ for @$items; return $s; }

1;
