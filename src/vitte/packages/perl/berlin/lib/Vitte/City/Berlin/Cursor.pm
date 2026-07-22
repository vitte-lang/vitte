package Vitte::City::Berlin::Cursor;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(append length_of);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(cursor cursor_position read_u8 remaining seek write_u8);

sub cursor {
    my ($buf) = @_;
    $buf->{cursor} ||= 0;
    return $buf;
}

sub cursor_position {
    return $_[0]->{cursor} || 0;
}

sub remaining {
    my ($buf) = @_;
    return length_of($buf) - cursor_position($buf);
}

sub seek {
    my ($buf, $position) = @_;
    die 'cursor out of range' if $position < 0 || $position > length_of($buf);
    $buf->{cursor} = $position;
    return $buf;
}

sub read_u8 {
    my ($buf) = @_;
    die 'read past end of buffer' if remaining($buf) <= 0;
    my $value = $buf->{bytes}->[cursor_position($buf)];
    $buf->{cursor}++;
    return $value;
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

1;
