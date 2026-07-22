package Vitte::City::Dakar::Sink;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    memory_sink
    sink_clear
    sink_drain
    sink_entries
    sink_last
    sink_size
    sink_write
    tee_sinks
);

sub memory_sink {
    return { entries => [] };
}

sub sink_write {
    my ($sink, $entry) = @_;
    push @{ $sink->{entries} }, $entry;
    return $sink;
}

sub sink_entries {
    return [ @{ $_[0]->{entries} || [] } ];
}

sub sink_last {
    my ($sink) = @_;
    return undef unless @{ $sink->{entries} || [] };
    return $sink->{entries}[-1];
}

sub sink_size {
    return scalar @{ $_[0]->{entries} || [] };
}

sub sink_clear {
    my ($sink) = @_;
    $sink->{entries} = [];
    return $sink;
}

sub sink_drain {
    my ($sink) = @_;
    my $entries = sink_entries($sink);
    sink_clear($sink);
    return $entries;
}

sub tee_sinks {
    my (@sinks) = @_;
    return {
        entries => [],
        write   => sub {
            my ($entry) = @_;
            sink_write($_, $entry) for @sinks;
            return $entry;
        },
    };
}

1;
