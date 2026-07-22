package Vitte::City::Cairo::Scalar;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    decode_scalar
    encode_scalar
    escape_scalar
    scalar_compare
    scalar_default
    scalar_is_bool
    scalar_is_integer
    scalar_is_null
    scalar_is_number
    scalar_lower
    scalar_to_bool
    scalar_to_number
    scalar_to_string
    scalar_trim
    scalar_type
    scalar_upper
    unescape_scalar
);

sub escape_scalar {
    my ($text) = @_;
    $text = '' unless defined $text;
    $text =~ s/\\/\\\\/g;
    $text =~ s/\r/\\r/g;
    $text =~ s/\n/\\n/g;
    $text =~ s/\t/\\t/g;
    $text =~ s/=/\\e/g;
    $text =~ s/;/\\s/g;
    $text =~ s/\|/\\p/g;
    return $text;
}

sub unescape_scalar {
    my ($text) = @_;
    $text = '' unless defined $text;
    $text =~ s/\\p/|/g;
    $text =~ s/\\s/;/g;
    $text =~ s/\\e/=/g;
    $text =~ s/\\t/\t/g;
    $text =~ s/\\n/\n/g;
    $text =~ s/\\r/\r/g;
    $text =~ s/\\\\/\\/g;
    return $text;
}

sub encode_scalar {
    return escape_scalar($_[0]);
}

sub decode_scalar {
    return unescape_scalar($_[0]);
}

sub scalar_is_null {
    return defined $_[0] ? 0 : 1;
}

sub scalar_is_bool {
    my ($value) = @_;
    return 0 unless defined $value;
    return $value =~ /\A(?:true|false|1|0|yes|no|on|off)\z/i ? 1 : 0;
}

sub scalar_is_integer {
    my ($value) = @_;
    return 0 unless defined $value;
    return $value =~ /\A[+-]?\d+\z/ ? 1 : 0;
}

sub scalar_is_number {
    my ($value) = @_;
    return 0 unless defined $value;
    return $value =~ /\A[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?\z/ ? 1 : 0;
}

sub scalar_type {
    my ($value) = @_;
    return 'null'    unless defined $value;
    return 'bool'    if scalar_is_bool($value);
    return 'integer' if scalar_is_integer($value);
    return 'number'  if scalar_is_number($value);
    return 'string';
}

sub scalar_to_bool {
    my ($value) = @_;
    return 0 unless defined $value;
    return 1 if $value =~ /\A(?:true|1|yes|on)\z/i;
    return 0 if $value =~ /\A(?:false|0|no|off)\z/i;
    return length($value) ? 1 : 0;
}

sub scalar_to_number {
    my ($value, $default) = @_;
    return scalar_is_number($value) ? 0 + $value : $default;
}

sub scalar_to_string {
    my ($value) = @_;
    return defined $value ? "$value" : '';
}

sub scalar_trim {
    my ($value) = @_;
    $value = scalar_to_string($value);
    $value =~ s/\A\s+//;
    $value =~ s/\s+\z//;
    return $value;
}

sub scalar_lower {
    return lc scalar_to_string($_[0]);
}

sub scalar_upper {
    return uc scalar_to_string($_[0]);
}

sub scalar_default {
    my ($value, $default) = @_;
    return defined $value && $value ne '' ? $value : $default;
}

sub scalar_compare {
    my ($left, $right) = @_;
    return scalar_to_string($left) cmp scalar_to_string($right);
}

1;
