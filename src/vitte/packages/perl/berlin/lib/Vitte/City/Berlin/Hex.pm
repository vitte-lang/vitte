package Vitte::City::Berlin::Hex;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(buffer);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(from_hex is_hex to_hex);

sub to_hex {
    my ($buf) = @_;
    return join('', map { sprintf('%02x', $_) } @{ $buf->{bytes} || [] });
}

sub is_hex {
    my ($hex) = @_;
    $hex = '' unless defined $hex;
    $hex =~ s/\s+//g;
    return length($hex) % 2 == 0 && $hex =~ /\A[0-9A-Fa-f]*\z/ ? 1 : 0;
}

sub from_hex {
    my ($hex) = @_;
    $hex = '' unless defined $hex;
    $hex =~ s/\s+//g;
    die 'invalid hex string' unless is_hex($hex);
    return buffer(map { hex($_) } ($hex =~ /(..)/g));
}

1;
