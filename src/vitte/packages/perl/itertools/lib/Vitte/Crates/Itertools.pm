package Vitte::Crates::Itertools;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(map_list filter_list fold_list chunks);

sub map_list { my ($items,$f)=@_; return [ map { $f->($_) } @$items ]; }
sub filter_list { my ($items,$f)=@_; return [ grep { $f->($_) } @$items ]; }
sub fold_list { my ($items,$init,$f)=@_; my $acc=$init; for my $x (@$items) { $acc=$f->($acc,$x); } return $acc; }
sub chunks { my ($items,$size)=@_; my @out; for (my $i=0; $i<@$items; $i+=$size) { push @out, [ @$items[$i .. (($i+$size-1)<$#$items ? $i+$size-1 : $#$items)] ]; } return \@out; }

1;
