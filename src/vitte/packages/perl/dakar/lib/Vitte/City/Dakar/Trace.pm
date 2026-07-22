package Vitte::City::Dakar::Trace;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    span_duration_ms
    span_add_field
    span_event
    span_finish
    span_fields
    span_id
    span_start
    span_to_hash
    trace_id
);

my $COUNTER = 0;

sub _id {
    my ($prefix) = @_;
    $COUNTER++;
    return join('-', $prefix, $$, time, $COUNTER);
}

sub trace_id {
    return _id('trace');
}

sub span_id {
    return _id('span');
}

sub span_start {
    my (%args) = @_;
    return {
        trace_id => $args{trace_id} || trace_id(),
        span_id  => $args{span_id} || span_id(),
        name     => $args{name} || 'span',
        start    => defined $args{start} ? $args{start} : time,
        end      => undef,
        events   => [],
        fields   => { %{ $args{fields} || {} } },
    };
}

sub span_event {
    my ($span, $name, %fields) = @_;
    push @{ $span->{events} }, {
        name      => $name,
        timestamp => time,
        fields    => { %fields },
    };
    return $span;
}

sub span_add_field {
    my ($span, $key, $value) = @_;
    $span->{fields}{$key} = $value;
    return $span;
}

sub span_fields {
    return { %{ $_[0]->{fields} || {} } };
}

sub span_finish {
    my ($span, %fields) = @_;
    $span->{end} = defined $fields{end} ? $fields{end} : time;
    $span->{fields} = { %{ $span->{fields} || {} }, %fields };
    return $span;
}

sub span_to_hash {
    my ($span) = @_;
    return {
        trace_id => $span->{trace_id},
        span_id  => $span->{span_id},
        name     => $span->{name},
        start    => $span->{start},
        end      => $span->{end},
        events   => [ @{ $span->{events} || [] } ],
        fields   => span_fields($span),
        duration_ms => span_duration_ms($span),
    };
}

sub span_duration_ms {
    my ($span) = @_;
    my $end = defined $span->{end} ? $span->{end} : time;
    return int(($end - $span->{start}) * 1000);
}

1;
