package Vitte::City::Berlin::View;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(buffer length_of slice);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(view view_length view_slice view_to_buffer);

sub view {
    my ($buf, $start, $count) = @_;
    $start ||= 0;
    $count = length_of($buf) - $start unless defined $count;
    die 'view out of range' if $start < 0 || $count < 0 || $start + $count > length_of($buf);
    return { buffer => $buf, start => $start, count => $count };
}

sub view_length {
    return $_[0]->{count};
}

sub view_slice {
    my ($v, $start, $count) = @_;
    return view($v->{buffer}, $v->{start} + $start, $count);
}

sub view_to_buffer {
    my ($v) = @_;
    return slice($v->{buffer}, $v->{start}, $v->{count});
}

1;
