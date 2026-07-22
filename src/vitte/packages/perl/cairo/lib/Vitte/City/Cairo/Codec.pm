package Vitte::City::Cairo::Codec;

use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use Exporter 'import';
use JSON::PP ();

use Vitte::City::Cairo::List qw(decode_list encode_list);
use Vitte::City::Cairo::Record qw(decode_record encode_record record_delete record_get record_keys record_set);
use Vitte::City::Cairo::Table qw(table table_add_row table_columns table_rows);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    canonical_json
    decode_document
    decode_lines
    decode_pairs
    decode_table
    document_delete
    document_get
    document_keys
    document_set
    encode_document
    encode_lines
    encode_pairs
    encode_table
    stable_hash
);

sub _json {
    return JSON::PP->new->ascii->canonical->allow_nonref;
}

sub canonical_json {
    my ($value) = @_;
    return _json()->encode($value);
}

sub stable_hash {
    my ($value) = @_;
    return sha256_hex(canonical_json($value));
}

sub encode_document {
    my ($document) = @_;
    return canonical_json($document || {});
}

sub decode_document {
    my ($text) = @_;
    return _json()->decode(defined $text && length $text ? $text : '{}');
}

sub encode_lines {
    my ($records) = @_;
    return join("\n", map { encode_record($_) } @{ $records || [] });
}

sub decode_lines {
    my ($text) = @_;
    return [] unless defined $text && length $text;
    return [ map { decode_record($_) } grep { length } split /\n/, $text ];
}

sub encode_pairs {
    my ($pairs) = @_;
    return encode_list([ map { $_->[0] . '=' . $_->[1] } @{ $pairs || [] } ]);
}

sub decode_pairs {
    my ($text) = @_;
    return [ map { [ split /=/, $_, 2 ] } @{ decode_list($text) } ];
}

sub encode_table {
    my ($table) = @_;
    return encode_document({
        columns => table_columns($table),
        rows    => table_rows($table),
    });
}

sub decode_table {
    my ($text) = @_;
    my $doc = decode_document($text);
    return table($doc->{columns}, $doc->{rows});
}

sub document_get {
    return record_get($_[0], $_[1], $_[2]);
}

sub document_set {
    return record_set($_[0], $_[1], $_[2]);
}

sub document_delete {
    return record_delete($_[0], $_[1]);
}

sub document_keys {
    return record_keys($_[0]);
}

1;
