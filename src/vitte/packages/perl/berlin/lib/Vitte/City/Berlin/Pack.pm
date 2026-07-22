package Vitte::City::Berlin::Pack;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(buffer buffer_bytes length_of);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    pack_i8
    pack_i16_be
    pack_i16_le
    pack_i32_be
    pack_i32_le
    pack_u8
    pack_u16_be
    pack_u16_le
    pack_u32_be
    pack_u32_le
    unpack_i8
    unpack_i16_be
    unpack_i16_le
    unpack_i32_be
    unpack_i32_le
    unpack_u8
    unpack_u16_be
    unpack_u16_le
    unpack_u32_be
    unpack_u32_le
    pack_bytes_be
    pack_bytes_le
    unpack_bytes_be
    unpack_bytes_le
);

my $U32_LIMIT = 4294967296;
my $I32_SIGN  = 2147483648;

sub _range {
    my ($value, $min, $max, $name) = @_;
    die "$name out of range" if !defined $value || $value < $min || $value > $max;
    return int($value);
}

sub _bytes_or_zero {
    my ($buf, $count) = @_;
    my @bytes = @{ buffer_bytes($buf) };
    push @bytes, 0 while @bytes < $count;
    return @bytes[0 .. $count - 1];
}

sub _twos {
    my ($value, $bits) = @_;
    my $limit = 2 ** $bits;
    $value += $limit if $value < 0;
    return $value;
}

sub _signed {
    my ($value, $bits) = @_;
    my $sign = 2 ** ($bits - 1);
    my $limit = 2 ** $bits;
    return $value >= $sign ? $value - $limit : $value;
}

sub pack_u8 { return buffer(_range($_[0], 0, 255, 'u8')) }
sub unpack_u8 { return (_bytes_or_zero($_[0], 1))[0] }
sub pack_i8 { return pack_u8(_twos(_range($_[0], -128, 127, 'i8'), 8)) }
sub unpack_i8 { return _signed(unpack_u8($_[0]), 8) }

sub pack_u16_be {
    my $v = _range($_[0], 0, 65535, 'u16');
    return buffer(($v >> 8) & 255, $v & 255);
}

sub pack_u16_le {
    my $v = _range($_[0], 0, 65535, 'u16');
    return buffer($v & 255, ($v >> 8) & 255);
}

sub unpack_u16_be {
    my @b = _bytes_or_zero($_[0], 2);
    return ($b[0] << 8) | $b[1];
}

sub unpack_u16_le {
    my @b = _bytes_or_zero($_[0], 2);
    return ($b[1] << 8) | $b[0];
}

sub pack_i16_be { return pack_u16_be(_twos(_range($_[0], -32768, 32767, 'i16'), 16)) }
sub pack_i16_le { return pack_u16_le(_twos(_range($_[0], -32768, 32767, 'i16'), 16)) }
sub unpack_i16_be { return _signed(unpack_u16_be($_[0]), 16) }
sub unpack_i16_le { return _signed(unpack_u16_le($_[0]), 16) }

sub pack_u32_be {
    my $v = _range($_[0], 0, $U32_LIMIT - 1, 'u32');
    return buffer(($v >> 24) & 255, ($v >> 16) & 255, ($v >> 8) & 255, $v & 255);
}

sub pack_u32_le {
    my $v = _range($_[0], 0, $U32_LIMIT - 1, 'u32');
    return buffer($v & 255, ($v >> 8) & 255, ($v >> 16) & 255, ($v >> 24) & 255);
}

sub unpack_u32_be {
    my @b = _bytes_or_zero($_[0], 4);
    return (($b[0] << 24) | ($b[1] << 16) | ($b[2] << 8) | $b[3]) & ($U32_LIMIT - 1);
}

sub unpack_u32_le {
    my @b = _bytes_or_zero($_[0], 4);
    return (($b[3] << 24) | ($b[2] << 16) | ($b[1] << 8) | $b[0]) & ($U32_LIMIT - 1);
}

sub pack_i32_be { return pack_u32_be(_twos(_range($_[0], -$I32_SIGN, $I32_SIGN - 1, 'i32'), 32)) }
sub pack_i32_le { return pack_u32_le(_twos(_range($_[0], -$I32_SIGN, $I32_SIGN - 1, 'i32'), 32)) }
sub unpack_i32_be { return _signed(unpack_u32_be($_[0]), 32) }
sub unpack_i32_le { return _signed(unpack_u32_le($_[0]), 32) }

sub pack_bytes_be {
    my (@values) = @_;
    return buffer(map { _range($_, 0, 255, 'byte') } @values);
}

sub pack_bytes_le {
    my (@values) = @_;
    return buffer(reverse map { _range($_, 0, 255, 'byte') } @values);
}

sub unpack_bytes_be {
    my ($buf) = @_;
    return buffer_bytes($buf);
}

sub unpack_bytes_le {
    my ($buf) = @_;
    return [ reverse @{ buffer_bytes($buf) } ];
}

1;
