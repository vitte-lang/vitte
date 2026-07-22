package Vitte::City::Berlin::Pack;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(buffer buffer_bytes);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(pack_i32_be pack_u16_be pack_u16_le pack_u32_be unpack_i32_be unpack_u16_be unpack_u16_le unpack_u32_be);

sub pack_u16_be { my ($v) = @_; return buffer(($v >> 8) & 255, $v & 255) }
sub pack_u16_le { my ($v) = @_; return buffer($v & 255, ($v >> 8) & 255) }
sub unpack_u16_be { my ($buf) = @_; my $b = buffer_bytes($buf); return (($b->[0] || 0) << 8) | ($b->[1] || 0) }
sub unpack_u16_le { my ($buf) = @_; my $b = buffer_bytes($buf); return (($b->[1] || 0) << 8) | ($b->[0] || 0) }

sub pack_u32_be {
    my ($v) = @_;
    return buffer(($v >> 24) & 255, ($v >> 16) & 255, ($v >> 8) & 255, $v & 255);
}

sub unpack_u32_be {
    my ($buf) = @_;
    my $b = buffer_bytes($buf);
    return (($b->[0] || 0) << 24) | (($b->[1] || 0) << 16) | (($b->[2] || 0) << 8) | ($b->[3] || 0);
}

sub pack_i32_be {
    my ($v) = @_;
    $v += 0x1_0000_0000 if $v < 0;
    return pack_u32_be($v);
}

sub unpack_i32_be {
    my ($buf) = @_;
    my $v = unpack_u32_be($buf);
    return $v >= 0x8000_0000 ? $v - 0x1_0000_0000 : $v;
}

1;
