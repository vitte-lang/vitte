package Vitte::City::Berlin::Hex;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(buffer buffer_bytes);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    from_hex
    from_hex_strict
    hex_dump
    hex_group
    hex_pairs
    is_hex
    normalize_hex
    to_hex
    to_hex_upper
);

sub normalize_hex {
    my ($hex) = @_;
    $hex = '' unless defined $hex;
    $hex =~ s/\s+//g;
    $hex =~ s/^0x//i;
    return lc $hex;
}

sub is_hex {
    my ($hex) = @_;
    $hex = normalize_hex($hex);
    return length($hex) % 2 == 0 && $hex =~ /\A[0-9a-f]*\z/ ? 1 : 0;
}

sub from_hex {
    my ($hex) = @_;
    $hex = normalize_hex($hex);
    die 'invalid hex string' unless is_hex($hex);
    return buffer(map { hex($_) } ($hex =~ /(..)/g));
}

sub from_hex_strict {
    my ($hex) = @_;
    die 'invalid strict hex string' unless defined $hex && $hex =~ /\A(?:[0-9A-Fa-f]{2})*\z/;
    return from_hex($hex);
}

sub to_hex {
    my ($buf) = @_;
    return join('', map { sprintf('%02x', $_) } @{ buffer_bytes($buf) });
}

sub to_hex_upper {
    my ($buf) = @_;
    return uc to_hex($buf);
}

sub hex_pairs {
    my ($buf) = @_;
    return [ map { sprintf('%02x', $_) } @{ buffer_bytes($buf) } ];
}

sub hex_group {
    my ($buf, $group_size, $separator) = @_;
    $group_size = 1 unless defined $group_size && $group_size > 0;
    $separator = ' ' unless defined $separator;
    my @pairs = @{ hex_pairs($buf) };
    my @groups;
    while (@pairs) {
        push @groups, join('', splice(@pairs, 0, $group_size));
    }
    return join($separator, @groups);
}

sub hex_dump {
    my ($buf, $width) = @_;
    $width = 16 unless defined $width && $width > 0;
    my @bytes = @{ buffer_bytes($buf) };
    my @lines;
    for (my $offset = 0; $offset < @bytes; $offset += $width) {
        my @chunk = @bytes[$offset .. ($offset + $width - 1 < $#bytes ? $offset + $width - 1 : $#bytes)];
        my $hex = join(' ', map { sprintf('%02x', $_) } @chunk);
        my $ascii = join('', map { $_ >= 32 && $_ <= 126 ? chr($_) : '.' } @chunk);
        push @lines, sprintf('%08x  %-*s  |%s|', $offset, $width * 3 - 1, $hex, $ascii);
    }
    return join("\n", @lines);
}

1;
