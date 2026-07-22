package Vitte::City::Lima;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(map_list filter_list fold_list chunks zip take unique flatten);

sub map_list {
    my ($list, $fn) = @_;
    return [ map { $fn->($_) } @{ $list || [] } ];
}

sub filter_list {
    my ($list, $fn) = @_;
    return [ grep { $fn->($_) } @{ $list || [] } ];
}

sub fold_list {
    my ($list, $init, $fn) = @_;
    my $acc = $init;
    $acc = $fn->($acc, $_) for @{ $list || [] };
    return $acc;
}

sub chunks {
    my ($list, $size) = @_;
    my @out;
    for (my $i = 0; $i < @{ $list || [] }; $i += $size) {
        push @out, [ @{ $list }[$i .. ($i + $size - 1 < @$list - 1 ? $i + $size - 1 : @$list - 1)] ];
    }
    return \@out;
}

sub zip {
    my ($a, $b) = @_;
    my @out;
    my $n = @$a < @$b ? @$a : @$b;
    push @out, [$a->[$_], $b->[$_]] for 0 .. $n - 1;
    return \@out;
}

sub take { my ($list, $n) = @_; return [ @{ $list || [] }[0 .. ($n - 1)] ] }
sub flatten { return [ map { ref($_) eq 'ARRAY' ? @$_ : $_ } @{ $_[0] || [] } ] }

sub unique {
    my ($list) = @_;
    my %seen;
    return [ grep { !$seen{$_}++ } @{ $list || [] } ];
}

1;
