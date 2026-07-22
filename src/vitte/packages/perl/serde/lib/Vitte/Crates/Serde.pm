package Vitte::Crates::Serde;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(serialize_value deserialize_value is_serialized);

sub serialize_value {
    my ($value) = @_;
    return 'null' unless defined $value;
    if (ref($value) eq 'ARRAY') {
        return '[' . join(',', map { serialize_value($_) } @$value) . ']';
    }
    if (ref($value) eq 'HASH') {
        return '{' . join(',', map { quote_string($_) . ':' . serialize_value($value->{$_}) } sort keys %$value) . '}';
    }
    return $value =~ /^-?\d+(?:\.\d+)?\z/ ? "$value" : quote_string($value);
}
sub deserialize_value {
    my ($text) = @_;
    return undef if !defined($text) || $text eq 'null';
    return $1 + 0 if $text =~ /\A(-?\d+(?:\.\d+)?)\z/;
    return $1 if $text =~ /\A"(.*)"\z/;
    return $text;
}
sub is_serialized { defined($_[0]) && $_[0] =~ /\A(?:null|".*"|-?\d|\[|\{)/ ? 1 : 0 }
sub quote_string { my ($s)=@_; $s =~ s/(["\\])/\\$1/g; return '"'.$s.'"'; }

1;
