package Vitte::City::Dakar::Metrics;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Dakar::Logger qw(entries);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    count_by_field
    count_by_level
    duration_seconds
    first_timestamp
    has_errors
    has_warnings
    last_timestamp
    levels_seen
    messages
    messages_by_level
    summarize
);

sub _entries {
    my ($source) = @_;
    return ref($source) eq 'ARRAY' ? @{$source} : entries($source);
}

sub count_by_level {
    my ($source) = @_;
    my %count;
    $count{$_->{level}}++ for _entries($source);
    return \%count;
}

sub count_by_field {
    my ($source, $field) = @_;
    my %count;
    for my $entry (_entries($source)) {
        next unless exists $entry->{fields}{$field};
        $count{ $entry->{fields}{$field} }++;
    }
    return \%count;
}

sub levels_seen {
    my ($source) = @_;
    my $count = count_by_level($source);
    return [ sort keys %{$count} ];
}

sub has_errors {
    my ($source) = @_;
    my $count = count_by_level($source);
    return ($count->{error} || 0) + ($count->{fatal} || 0) ? 1 : 0;
}

sub has_warnings {
    my ($source) = @_;
    my $count = count_by_level($source);
    return ($count->{warn} || 0) ? 1 : 0;
}

sub first_timestamp {
    my ($source) = @_;
    my @entries = sort { $a->{timestamp} <=> $b->{timestamp} } _entries($source);
    return @entries ? $entries[0]->{timestamp} : undef;
}

sub last_timestamp {
    my ($source) = @_;
    my @entries = sort { $a->{timestamp} <=> $b->{timestamp} } _entries($source);
    return @entries ? $entries[-1]->{timestamp} : undef;
}

sub duration_seconds {
    my ($source) = @_;
    my $first = first_timestamp($source);
    my $last = last_timestamp($source);
    return 0 unless defined $first && defined $last;
    return $last - $first;
}

sub messages {
    my ($source) = @_;
    return [ map { $_->{message} } _entries($source) ];
}

sub messages_by_level {
    my ($source, $level) = @_;
    return [ map { $_->{message} } grep { $_->{level} eq $level } _entries($source) ];
}

sub summarize {
    my ($source) = @_;
    my @entries = _entries($source);
    return {
        total       => scalar @entries,
        by_level    => count_by_level(\@entries),
        levels      => levels_seen(\@entries),
        first_timestamp => first_timestamp(\@entries),
        last_timestamp  => last_timestamp(\@entries),
        duration_seconds => duration_seconds(\@entries),
        has_errors  => has_errors(\@entries),
        has_warnings => has_warnings(\@entries),
    };
}

1;
