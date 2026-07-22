package Vitte::City::Berlin;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(buffer from_string append length_of slice to_hex from_hex concat_buffers);

sub buffer {
    my @bytes = @_;
    return { bytes => [ map { int($_) & 255 } @bytes ], cursor => 0 };
}

sub from_string {
    my ($text) = @_;
    $text = '' unless defined $text;
    return buffer(unpack('C*', $text));
}

sub append {
    my ($buf, @bytes) = @_;
    push @{ $buf->{bytes} }, map { int($_) & 255 } @bytes;
    return $buf;
}

sub length_of {
    my ($buf) = @_;
    return scalar @{ $buf->{bytes} || [] };
}

sub slice {
    my ($buf, $start, $count) = @_;
    $start ||= 0;
    $count = length_of($buf) - $start unless defined $count;
    return buffer(@{ $buf->{bytes} }[$start .. $start + $count - 1]);
}

sub to_hex {
    my ($buf) = @_;
    return join('', map { sprintf('%02x', $_) } @{ $buf->{bytes} || [] });
}

sub from_hex {
    my ($hex) = @_;
    $hex =~ s/\s+//g;
    die 'hex string has odd length' if length($hex) % 2;
    return buffer(map { hex($_) } ($hex =~ /(..)/g));
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
