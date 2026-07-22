package Vitte::City::Dakar::Entry;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Dakar::Level qw(normalize_level);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    entry
    entry_field
    entry_fields
    entry_level
    entry_message
    entry_timestamp
    entry_to_hash
    entry_with_field
    event
    fields
    level
    message
    timestamp
);

sub entry {
    my (%args) = @_;
    return {
        level     => normalize_level($args{level}),
        message   => defined $args{message} ? "$args{message}" : '',
        timestamp => defined $args{timestamp} ? $args{timestamp} : time,
        fields    => { %{ $args{fields} || {} } },
    };
}

sub event {
    return entry(@_);
}

sub entry_level {
    return $_[0]->{level};
}

sub level {
    return entry_level($_[0]);
}

sub entry_message {
    return $_[0]->{message};
}

sub message {
    return entry_message($_[0]);
}

sub entry_timestamp {
    return $_[0]->{timestamp};
}

sub timestamp {
    return entry_timestamp($_[0]);
}

sub entry_fields {
    return { %{ $_[0]->{fields} || {} } };
}

sub fields {
    return entry_fields($_[0]);
}

sub entry_field {
    my ($entry, $key, $default) = @_;
    return exists $entry->{fields}{$key} ? $entry->{fields}{$key} : $default;
}

sub entry_with_field {
    my ($entry, $key, $value) = @_;
    my %fields = %{ $entry->{fields} || {} };
    $fields{$key} = $value;
    return entry(
        level     => $entry->{level},
        message   => $entry->{message},
        timestamp => $entry->{timestamp},
        fields    => \%fields,
    );
}

sub entry_to_hash {
    my ($entry) = @_;
    return {
        level     => $entry->{level},
        message   => $entry->{message},
        timestamp => $entry->{timestamp},
        fields    => entry_fields($entry),
    };
}

1;
