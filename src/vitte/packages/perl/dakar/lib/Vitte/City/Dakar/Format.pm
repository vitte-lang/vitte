package Vitte::City::Dakar::Format;

use strict;
use warnings;
use Exporter 'import';
use JSON::PP ();

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    format_json
    format_kv
    format_csv
    format_lines
    format_level_badge
    format_pretty
    format_text
    parse_kv
);

sub _json {
    return JSON::PP->new->ascii->canonical;
}

sub _fields_text {
    my ($fields) = @_;
    return join(' ', map { "$_=$fields->{$_}" } sort keys %{ $fields || {} });
}

sub format_text {
    my ($entry) = @_;
    my $fields = _fields_text($entry->{fields});
    return join(' ', grep { length } (
        '[' . $entry->{level} . ']',
        $entry->{message} || '',
        $fields,
    ));
}

sub format_json {
    my ($entry) = @_;
    return _json()->encode($entry);
}

sub format_kv {
    my ($entry) = @_;
    my %flat = (
        level     => $entry->{level},
        message   => $entry->{message},
        timestamp => $entry->{timestamp},
        %{ $entry->{fields} || {} },
    );
    return join(' ', map { "$_=$flat{$_}" } sort keys %flat);
}

sub format_csv {
    my ($entry) = @_;
    my @values = ($entry->{timestamp}, $entry->{level}, $entry->{message});
    for my $value (@values) {
        $value = '' unless defined $value;
        $value =~ s/"/""/g;
        $value = qq("$value") if $value =~ /[",\n]/;
    }
    return join(',', @values);
}

sub format_level_badge {
    my ($entry) = @_;
    return '[' . uc($entry->{level}) . ']';
}

sub parse_kv {
    my ($text) = @_;
    my %out;
    for my $part (split /\s+/, $text || '') {
        next unless length $part;
        my ($key, $value) = split /=/, $part, 2;
        $out{$key} = defined $value ? $value : '';
    }
    return \%out;
}

sub format_lines {
    my ($entries, $formatter) = @_;
    $formatter ||= \&format_text;
    return join("\n", map { $formatter->($_) } @{ $entries || [] });
}

sub format_pretty {
    my ($entry) = @_;
    return sprintf(
        "%s %-5s %s %s",
        $entry->{timestamp},
        uc($entry->{level}),
        $entry->{message} || '',
        _fields_text($entry->{fields}),
    );
}

1;
