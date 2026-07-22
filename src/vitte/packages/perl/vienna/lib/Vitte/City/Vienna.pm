package Vitte::City::Vienna;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(sum mean median minmax clamp lerp percentile variance);

sub sum {
    my ($values) = @_;
    my $total = 0;
    $total += $_ for @{ $values || [] };
    return $total;
}

sub mean { my ($values) = @_; return @$values ? sum($values) / @$values : 0 }

sub median {
    my ($values) = @_;
    return 0 unless @$values;
    my @v = sort { $a <=> $b } @$values;
    my $mid = int(@v / 2);
    return @v % 2 ? $v[$mid] : ($v[$mid - 1] + $v[$mid]) / 2;
}

sub minmax {
    my ($values) = @_;
    my ($min, $max) = ($values->[0], $values->[0]);
    for my $v (@$values) { $min = $v if $v < $min; $max = $v if $v > $max; }
    return ($min, $max);
}

sub clamp { my ($v, $lo, $hi) = @_; return $v < $lo ? $lo : $v > $hi ? $hi : $v }
sub lerp { my ($a, $b, $t) = @_; return $a + (($b - $a) * $t) }

sub percentile {
    my ($values, $p) = @_;
    my @v = sort { $a <=> $b } @$values;
    return 0 unless @v;
    return $v[int(clamp($p, 0, 1) * (@v - 1))];
}

sub variance {
    my ($values) = @_;
    my $m = mean($values);
    return mean([ map { ($_ - $m) * ($_ - $m) } @$values ]);
}

1;
