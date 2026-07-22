package Vitte::City::Dakar::Filter;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Dakar::Level qw(level_enabled normalize_level);
use Vitte::City::Dakar::Logger qw(entries);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    filter_between
    filter_field
    filter_level
    filter_limit
    filter_message
    filter_min_level
    filter_predicate
    filter_recent
);

sub _entries {
    my ($source) = @_;
    return ref($source) eq 'ARRAY' ? @{$source} : entries($source);
}

sub filter_level {
    my ($source, $level) = @_;
    $level = normalize_level($level);
    return [ grep { $_->{level} eq $level } _entries($source) ];
}

sub filter_min_level {
    my ($source, $level) = @_;
    $level = normalize_level($level);
    return [ grep { level_enabled($_->{level}, $level) } _entries($source) ];
}

sub filter_message {
    my ($source, $pattern) = @_;
    return [ grep { ($_->{message} || '') =~ /$pattern/ } _entries($source) ];
}

sub filter_field {
    my ($source, $key, $value) = @_;
    return [
        grep {
            exists $_->{fields}{$key}
                && (!defined $value || "$_->{fields}{$key}" eq "$value")
        } _entries($source)
    ];
}

sub filter_between {
    my ($source, $start, $end) = @_;
    $start = 0 unless defined $start;
    $end = time unless defined $end;
    return [ grep { $_->{timestamp} >= $start && $_->{timestamp} <= $end } _entries($source) ];
}

sub filter_predicate {
    my ($source, $predicate) = @_;
    return [ grep { $predicate->($_) } _entries($source) ];
}

sub filter_recent {
    my ($source, $seconds) = @_;
    $seconds = 60 unless defined $seconds;
    return filter_between($source, time - $seconds, time);
}

sub filter_limit {
    my ($source, $count) = @_;
    $count = 0 unless defined $count && $count > 0;
    my @entries = _entries($source);
    @entries = @entries[-$count .. -1] if @entries > $count;
    return \@entries;
}

1;
