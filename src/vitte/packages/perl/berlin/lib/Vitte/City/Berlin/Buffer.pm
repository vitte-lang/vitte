package Vitte::City::Berlin::Buffer;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    append buffer buffer_bytes byte_at clear concat_buffers from_string is_empty
    length_of resize reverse_bytes slice to_string
);

sub buffer {
    my @bytes = @_;
    return { bytes => [ map { int($_) & 255 } @bytes ], cursor => 0 };
}

sub from_string {
    my ($text) = @_;
    $text = '' unless defined $text;
    return buffer(unpack('C*', $text));
}

sub to_string {
    my ($buf) = @_;
    return pack('C*', @{ $buf->{bytes} || [] });
}

sub append {
    my ($buf, @bytes) = @_;
    push @{ $buf->{bytes} }, map { int($_) & 255 } @bytes;
    return $buf;
}

sub buffer_bytes {
    my ($buf) = @_;
    return [ @{ $buf->{bytes} || [] } ];
}

sub length_of {
    my ($buf) = @_;
    return scalar @{ $buf->{bytes} || [] };
}

sub is_empty {
    return length_of($_[0]) == 0 ? 1 : 0;
}

sub byte_at {
    my ($buf, $index) = @_;
    die 'byte index out of range' if $index < 0 || $index >= length_of($buf);
    return $buf->{bytes}->[$index];
}

sub slice {
    my ($buf, $start, $count) = @_;
    $start ||= 0;
    $count = length_of($buf) - $start unless defined $count;
    return buffer() if $count <= 0;
    return buffer(@{ $buf->{bytes} }[$start .. $start + $count - 1]);
}

sub clear {
    my ($buf) = @_;
    $buf->{bytes} = [];
    $buf->{cursor} = 0;
    return $buf;
}

sub resize {
    my ($buf, $size, $fill) = @_;
    $fill = 0 unless defined $fill;
    while (length_of($buf) < $size) {
        append($buf, $fill);
    }
    splice @{ $buf->{bytes} }, $size if length_of($buf) > $size;
    $buf->{cursor} = $size if $buf->{cursor} > $size;
    return $buf;
}

sub reverse_bytes {
    my ($buf) = @_;
    return buffer(reverse @{ $buf->{bytes} || [] });
}

sub concat_buffers {
    my @buffers = @_;
    my @bytes;
    for my $buf (@buffers) {
        push @bytes, @{ $buf->{bytes} || [] };
    }
    return buffer(@bytes);
}

1;
