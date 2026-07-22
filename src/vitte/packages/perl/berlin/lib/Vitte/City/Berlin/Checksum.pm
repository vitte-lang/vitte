package Vitte::City::Berlin::Checksum;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(buffer_bytes);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(adler32 checksum8 crc32);

sub checksum8 {
    my ($buf) = @_;
    my $sum = 0;
    $sum = ($sum + $_) & 255 for @{ buffer_bytes($buf) };
    return $sum;
}

sub adler32 {
    my ($buf) = @_;
    my ($a, $b) = (1, 0);
    for my $byte (@{ buffer_bytes($buf) }) {
        $a = ($a + $byte) % 65521;
        $b = ($b + $a) % 65521;
    }
    return ($b << 16) | $a;
}

sub crc32 {
    my ($buf) = @_;
    my $crc = 0xffffffff;
    for my $byte (@{ buffer_bytes($buf) }) {
        $crc ^= $byte;
        for (1 .. 8) {
            $crc = ($crc & 1) ? (($crc >> 1) ^ 0xedb88320) : ($crc >> 1);
        }
    }
    return $crc ^ 0xffffffff;
}

1;
