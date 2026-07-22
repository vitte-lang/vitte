package Vitte::City::Berlin::Checksum;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(buffer_bytes);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    adler32
    checksum8
    checksum16
    crc16_ccitt
    crc32
    fletcher16
    fnv1a32
    parity8
    xor8
);

sub checksum8 {
    my ($buf) = @_;
    my $sum = 0;
    $sum = ($sum + $_) & 255 for @{ buffer_bytes($buf) };
    return $sum;
}

sub checksum16 {
    my ($buf) = @_;
    my $sum = 0;
    $sum = ($sum + $_) & 65535 for @{ buffer_bytes($buf) };
    return $sum;
}

sub xor8 {
    my ($buf) = @_;
    my $xor = 0;
    $xor ^= $_ for @{ buffer_bytes($buf) };
    return $xor & 255;
}

sub parity8 {
    my ($buf) = @_;
    my $bits = 0;
    for my $byte (@{ buffer_bytes($buf) }) {
        for (0 .. 7) {
            $bits ^= ($byte >> $_) & 1;
        }
    }
    return $bits;
}

sub adler32 {
    my ($buf) = @_;
    my ($a, $b) = (1, 0);
    for my $byte (@{ buffer_bytes($buf) }) {
        $a = ($a + $byte) % 65521;
        $b = ($b + $a) % 65521;
    }
    return (($b << 16) | $a) & 4294967295;
}

sub fletcher16 {
    my ($buf) = @_;
    my ($a, $b) = (0, 0);
    for my $byte (@{ buffer_bytes($buf) }) {
        $a = ($a + $byte) % 255;
        $b = ($b + $a) % 255;
    }
    return (($b << 8) | $a) & 65535;
}

sub crc16_ccitt {
    my ($buf, $initial) = @_;
    my $crc = defined $initial ? $initial & 65535 : 65535;
    for my $byte (@{ buffer_bytes($buf) }) {
        $crc ^= ($byte << 8);
        for (1 .. 8) {
            $crc = ($crc & 32768) ? (($crc << 1) ^ 4129) : ($crc << 1);
            $crc &= 65535;
        }
    }
    return $crc;
}

sub crc32 {
    my ($buf) = @_;
    my $crc = 4294967295;
    for my $byte (@{ buffer_bytes($buf) }) {
        $crc ^= $byte;
        for (1 .. 8) {
            $crc = ($crc & 1) ? (($crc >> 1) ^ 3988292384) : ($crc >> 1);
            $crc &= 4294967295;
        }
    }
    return ($crc ^ 4294967295) & 4294967295;
}

sub fnv1a32 {
    my ($buf) = @_;
    my $hash = 2166136261;
    for my $byte (@{ buffer_bytes($buf) }) {
        $hash ^= $byte;
        $hash = ($hash * 16777619) & 4294967295;
    }
    return $hash;
}

1;
