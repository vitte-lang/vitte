package Vitte::City::Cairo::List;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Cairo::Scalar qw(decode_scalar encode_scalar);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    decode_list
    encode_list
    list
    list_compact
    list_contains
    list_count
    list_flatten
    list_index_of
    list_join
    list_pop
    list_push
    list_reverse
    list_shift
    list_slice
    list_sort
    list_unique
    list_unshift
);

sub list {
    my (@items) = @_;
    return [ @items ];
}

sub encode_list {
    my ($items) = @_;
    return join(';', map { encode_scalar($_) } @{ $items || [] });
}

sub decode_list {
    my ($text) = @_;
    return [] unless defined $text && length $text;
    return [ map { decode_scalar($_) } split /;/, $text ];
}

sub list_push {
    my ($items, @values) = @_;
    push @{ $items }, @values;
    return $items;
}

sub list_pop {
    my ($items) = @_;
    return pop @{ $items };
}

sub list_shift {
    my ($items) = @_;
    return shift @{ $items };
}

sub list_unshift {
    my ($items, @values) = @_;
    unshift @{ $items }, @values;
    return $items;
}

sub list_unique {
    my ($items) = @_;
    my %seen;
    return [ grep { !$seen{defined $_ ? $_ : ''}++ } @{ $items || [] } ];
}

sub list_sort {
    my ($items, $direction) = @_;
    my @sorted = sort { "$a" cmp "$b" } @{ $items || [] };
    @sorted = reverse @sorted if defined $direction && $direction eq 'desc';
    return \@sorted;
}

sub list_reverse {
    my ($items) = @_;
    return [ reverse @{ $items || [] } ];
}

sub list_slice {
    my ($items, $start, $count) = @_;
    my $length = scalar @{ $items || [] };
    $start = 0 unless defined $start;
    die 'list slice out of range' if $start < 0 || $start > $length;
    $count = $length - $start unless defined $count;
    die 'list slice out of range' if $count < 0 || $start + $count > $length;
    return [] if $count == 0;
    return [ @{ $items }[$start .. $start + $count - 1] ];
}

sub list_join {
    my ($items, $separator) = @_;
    $separator = ',' unless defined $separator;
    return join($separator, @{ $items || [] });
}

sub list_contains {
    my ($items, $value) = @_;
    return list_index_of($items, $value) >= 0 ? 1 : 0;
}

sub list_index_of {
    my ($items, $value) = @_;
    for my $i (0 .. $#{ $items || [] }) {
        my $item = $items->[$i];
        return $i if defined $item && defined $value ? "$item" eq "$value" : !defined $item && !defined $value;
    }
    return -1;
}

sub list_count {
    my ($items) = @_;
    return scalar @{ $items || [] };
}

sub list_compact {
    my ($items) = @_;
    return [ grep { defined $_ && $_ ne '' } @{ $items || [] } ];
}

sub list_flatten {
    my ($items) = @_;
    my @out;
    for my $item (@{ $items || [] }) {
        if (ref($item) eq 'ARRAY') {
            push @out, @{ list_flatten($item) };
        }
        else {
            push @out, $item;
        }
    }
    return \@out;
}

1;
