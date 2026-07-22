package Vitte::City::Lyon::Context;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    context
    context_chain
    context_delete
    context_get
    context_has
    context_keys
    context_merge
    context_push
    context_set
    context_to_hash
    context_values
);

sub context {
    my (%values) = @_;
    return {
        values => { %values },
        chain  => [],
    };
}

sub context_get {
    my ($context, $key, $default) = @_;
    return exists $context->{values}{$key} ? $context->{values}{$key} : $default;
}

sub context_set {
    my ($context, $key, $value) = @_;
    $context->{values}{$key} = $value;
    return $context;
}

sub context_has {
    my ($context, $key) = @_;
    return exists $context->{values}{$key} ? 1 : 0;
}

sub context_delete {
    my ($context, $key) = @_;
    return delete $context->{values}{$key};
}

sub context_keys {
    return [ sort keys %{ $_[0]->{values} || {} } ];
}

sub context_values {
    my ($context) = @_;
    return [ map { $context->{values}{$_} } @{ context_keys($context) } ];
}

sub context_merge {
    my (@contexts) = @_;
    my $merged = context();
    for my $ctx (@contexts) {
        $merged->{values} = { %{ $merged->{values} }, %{ $ctx->{values} || {} } };
        push @{ $merged->{chain} }, @{ $ctx->{chain} || [] };
    }
    return $merged;
}

sub context_push {
    my ($context, $label, %fields) = @_;
    push @{ $context->{chain} }, {
        label  => $label,
        fields => { %fields },
    };
    return $context;
}

sub context_chain {
    return [ @{ $_[0]->{chain} || [] } ];
}

sub context_to_hash {
    my ($context) = @_;
    return {
        values => { %{ $context->{values} || {} } },
        chain  => context_chain($context),
    };
}

1;
