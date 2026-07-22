package Vitte::City::Cairo;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(encode_scalar decode_scalar encode_record decode_record encode_list decode_list);

sub _escape {
    my ($text) = @_;
    $text = '' unless defined $text;
    $text =~ s/\\/\\\\/g;
    $text =~ s/\n/\\n/g;
    $text =~ s/=/\\e/g;
    $text =~ s/;/\\s/g;
    return $text;
}

sub _unescape {
    my ($text) = @_;
    $text =~ s/\\s/;/g;
    $text =~ s/\\e/=/g;
    $text =~ s/\\n/\n/g;
    $text =~ s/\\\\/\\/g;
    return $text;
}

sub encode_scalar { return _escape($_[0]) }
sub decode_scalar { return _unescape($_[0]) }

sub encode_record {
    my ($record) = @_;
    return join(';', map { _escape($_) . '=' . _escape($record->{$_}) } sort keys %{ $record || {} });
}

sub decode_record {
    my ($text) = @_;
    my %record;
    for my $part (split /;/, $text || '') {
        next unless length $part;
        my ($key, $value) = split /=/, $part, 2;
        $record{_unescape($key)} = _unescape(defined $value ? $value : '');
    }
    return \%record;
}

sub encode_list { return join(';', map { _escape($_) } @{ $_[0] || [] }) }
sub decode_list { return [ map { _unescape($_) } split /;/, $_[0] || '' ] }

1;
