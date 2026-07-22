package Vitte::City::Berlin::Cursor;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(
    append
    buffer
    buffer_bytes
    length_of
    slice
);
use Vitte::City::Berlin::Pack qw(
    pack_u16_be
    pack_u16_le
    pack_u32_be
    pack_u32_le
    unpack_u16_be
    unpack_u16_le
    unpack_u32_be
    unpack_u32_le
);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    cursor
    cursor_at_end
    cursor_position
    peek_u8
    read_bytes
    read_u8
    read_u16_be
    read_u16_le
    read_u32_be
    read_u32_le
    remaining
    rewind
    seek
    skip
    write_bytes
    write_u8
    write_u16_be
    write_u16_le
    write_u32_be
    write_u32_le
);

sub cursor {
    my ($buf) = @_;
    $buf->{cursor} = 0 unless defined $buf->{cursor};
    return $buf;
}

sub cursor_position {
    return $_[0]->{cursor} || 0;
}

sub remaining {
    my ($buf) = @_;
    return length_of($buf) - cursor_position($buf);
}

sub cursor_at_end {
    return remaining($_[0]) == 0 ? 1 : 0;
}

sub seek {
    my ($buf, $position) = @_;
    die 'cursor out of range' if !defined $position || $position < 0 || $position > length_of($buf);
    $buf->{cursor} = $position;
    return $buf;
}

sub skip {
    my ($buf, $amount) = @_;
    $amount = 0 unless defined $amount;
    return &seek($buf, cursor_position($buf) + $amount);
}

sub rewind {
    my ($buf) = @_;
    return &seek($buf, 0);
}

sub peek_u8 {
    my ($buf) = @_;
    die 'read past end of buffer' if remaining($buf) <= 0;
    return $buf->{bytes}->[cursor_position($buf)];
}

sub read_u8 {
    my ($buf) = @_;
    my $value = peek_u8($buf);
    $buf->{cursor}++;
    return $value;
}

sub read_bytes {
    my ($buf, $count) = @_;
    die 'negative read size' if !defined $count || $count < 0;
    die 'read past end of buffer' if $count > remaining($buf);
    my $out = slice($buf, cursor_position($buf), $count);
    $buf->{cursor} += $count;
    return $out;
}

sub write_u8 {
    my ($buf, $value) = @_;
    my $pos = cursor_position($buf);
    if ($pos == length_of($buf)) {
        append($buf, $value);
    } else {
        $buf->{bytes}->[$pos] = int($value) & 255;
    }
    $buf->{cursor} = $pos + 1;
    return $buf;
}

sub write_bytes {
    my ($buf, $bytes) = @_;
    for my $byte (@{ buffer_bytes($bytes) }) {
        write_u8($buf, $byte);
    }
    return $buf;
}

sub read_u16_be { return unpack_u16_be(read_bytes($_[0], 2)) }
sub read_u16_le { return unpack_u16_le(read_bytes($_[0], 2)) }
sub read_u32_be { return unpack_u32_be(read_bytes($_[0], 4)) }
sub read_u32_le { return unpack_u32_le(read_bytes($_[0], 4)) }

sub write_u16_be { return write_bytes($_[0], pack_u16_be($_[1])) }
sub write_u16_le { return write_bytes($_[0], pack_u16_le($_[1])) }
sub write_u32_be { return write_bytes($_[0], pack_u32_be($_[1])) }
sub write_u32_le { return write_bytes($_[0], pack_u32_le($_[1])) }

1;
