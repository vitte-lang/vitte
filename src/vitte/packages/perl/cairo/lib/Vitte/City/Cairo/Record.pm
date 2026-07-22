package Vitte::City::Cairo::Record;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Cairo::Scalar qw(decode_scalar encode_scalar);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    decode_record
    encode_record
    record
    record_apply_defaults
    record_delete
    record_diff
    record_get
    record_has
    record_keys
    record_merge
    record_omit
    record_pairs
    record_project
    record_rename
    record_require
    record_set
    record_to_query
    record_values
);

sub record {
    my (%pairs) = @_;
    return { %pairs };
}

sub encode_record {
    my ($record) = @_;
    return join(';', map { encode_scalar($_) . '=' . encode_scalar($record->{$_}) } sort keys %{ $record || {} });
}

sub decode_record {
    my ($text) = @_;
    my %record;
    for my $part (split /;/, $text || '') {
        next unless length $part;
        my ($key, $value) = split /=/, $part, 2;
        $record{decode_scalar($key)} = decode_scalar(defined $value ? $value : '');
    }
    return \%record;
}

sub record_get {
    my ($record, $key, $default) = @_;
    return exists $record->{$key} ? $record->{$key} : $default;
}

sub record_set {
    my ($record, $key, $value) = @_;
    $record->{$key} = $value;
    return $record;
}

sub record_has {
    my ($record, $key) = @_;
    return exists $record->{$key} ? 1 : 0;
}

sub record_delete {
    my ($record, $key) = @_;
    my $value = delete $record->{$key};
    return $value;
}

sub record_keys {
    my ($record) = @_;
    return [ sort keys %{ $record || {} } ];
}

sub record_values {
    my ($record) = @_;
    return [ map { $record->{$_} } @{ record_keys($record) } ];
}

sub record_pairs {
    my ($record) = @_;
    return [ map { [ $_, $record->{$_} ] } @{ record_keys($record) } ];
}

sub record_merge {
    my (@records) = @_;
    my %merged;
    for my $record (@records) {
        %merged = (%merged, %{ $record || {} });
    }
    return \%merged;
}

sub record_project {
    my ($record, $keys) = @_;
    my %out;
    for my $key (@{ $keys || [] }) {
        $out{$key} = $record->{$key} if exists $record->{$key};
    }
    return \%out;
}

sub record_omit {
    my ($record, $keys) = @_;
    my %omit = map { $_ => 1 } @{ $keys || [] };
    my %out = map { $_ => $record->{$_} } grep { !$omit{$_} } keys %{ $record || {} };
    return \%out;
}

sub record_require {
    my ($record, $keys) = @_;
    my @missing = grep { !exists $record->{$_} || !defined $record->{$_} || $record->{$_} eq '' } @{ $keys || [] };
    return \@missing;
}

sub record_apply_defaults {
    my ($record, $defaults) = @_;
    my %out = %{ $record || {} };
    for my $key (keys %{ $defaults || {} }) {
        $out{$key} = $defaults->{$key} unless exists $out{$key} && defined $out{$key} && $out{$key} ne '';
    }
    return \%out;
}

sub record_rename {
    my ($record, $mapping) = @_;
    my %out = %{ $record || {} };
    for my $from (keys %{ $mapping || {} }) {
        next unless exists $out{$from};
        my $to = $mapping->{$from};
        $out{$to} = delete $out{$from};
    }
    return \%out;
}

sub record_diff {
    my ($left, $right) = @_;
    my %keys = map { $_ => 1 } (keys %{ $left || {} }, keys %{ $right || {} });
    my %diff;
    for my $key (sort keys %keys) {
        my $a = $left->{$key};
        my $b = $right->{$key};
        next if defined $a && defined $b && "$a" eq "$b";
        next if !defined $a && !defined $b;
        $diff{$key} = { left => $a, right => $b };
    }
    return \%diff;
}

sub record_to_query {
    my ($record) = @_;
    return join('&', map { encode_scalar($_) . '=' . encode_scalar($record->{$_}) } @{ record_keys($record) });
}

1;
