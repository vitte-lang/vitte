package Vitte::City::Berlin::View;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(
    buffer_compare
    buffer_equals
    byte_at
    contains_bytes
    index_of
    length_of
    slice
);
use Vitte::City::Berlin::Hex qw(to_hex);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    view
    view_byte_at
    view_contains
    view_end
    view_equals
    view_index_of
    view_is_empty
    view_length
    view_slice
    view_start
    view_to_buffer
    view_to_hex
);

sub view {
    my ($buf, $start, $count) = @_;
    $start = 0 unless defined $start;
    $count = length_of($buf) - $start unless defined $count;
    die 'view out of range' if $start < 0 || $count < 0 || $start + $count > length_of($buf);
    return {
        buffer => $buf,
        start  => $start,
        count  => $count,
    };
}

sub view_start {
    return $_[0]->{start};
}

sub view_end {
    return $_[0]->{start} + $_[0]->{count};
}

sub view_length {
    return $_[0]->{count};
}

sub view_is_empty {
    return view_length($_[0]) == 0 ? 1 : 0;
}

sub view_slice {
    my ($v, $start, $count) = @_;
    $start = 0 unless defined $start;
    $count = view_length($v) - $start unless defined $count;
    die 'view slice out of range' if $start < 0 || $count < 0 || $start + $count > view_length($v);
    return view($v->{buffer}, $v->{start} + $start, $count);
}

sub view_to_buffer {
    my ($v) = @_;
    return slice($v->{buffer}, $v->{start}, $v->{count});
}

sub view_byte_at {
    my ($v, $index) = @_;
    die 'view byte index out of range' if !defined $index || $index < 0 || $index >= view_length($v);
    return byte_at($v->{buffer}, $v->{start} + $index);
}

sub view_to_hex {
    my ($v) = @_;
    return to_hex(view_to_buffer($v));
}

sub view_equals {
    my ($left, $right) = @_;
    return buffer_equals(view_to_buffer($left), view_to_buffer($right));
}

sub view_index_of {
    my ($v, $needle, $start) = @_;
    $start = 0 unless defined $start;
    my $pos = index_of(view_to_buffer($v), $needle, $start);
    return $pos;
}

sub view_contains {
    return view_index_of($_[0], $_[1], $_[2]) >= 0 ? 1 : 0;
}

1;
