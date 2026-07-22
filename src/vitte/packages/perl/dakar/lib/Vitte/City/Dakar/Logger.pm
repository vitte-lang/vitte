package Vitte::City::Dakar::Logger;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Dakar::Entry qw(entry entry_to_hash);
use Vitte::City::Dakar::Level qw(level_enabled normalize_level);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    clear_entries
    clear_fields
    debug
    entries
    entry_count
    error
    fatal
    info
    logger_capacity
    logger_field
    last_entry
    log_event
    log_many
    logger
    logger_clone
    logger_fields
    logger_has_field
    logger_level
    logger_name
    logger_snapshot
    logger_restore
    prune_entries
    reset_logger
    set_level
    set_name
    set_capacity
    trace
    warn
    with_field
    with_fields
    without_field
);

sub logger {
    my ($level, %options) = @_;
    return {
        level   => normalize_level($level || 'info'),
        name    => $options{name} || 'dakar',
        fields  => { %{ $options{fields} || {} } },
        entries  => [],
        capacity => $options{capacity},
    };
}

sub logger_clone {
    my ($logger) = @_;
    return {
        level   => $logger->{level},
        name    => $logger->{name},
        fields  => { %{ $logger->{fields} || {} } },
        entries => [ map { entry_to_hash($_) } @{ $logger->{entries} || [] } ],
        capacity => $logger->{capacity},
    };
}

sub logger_level {
    return $_[0]->{level};
}

sub set_level {
    my ($logger, $level) = @_;
    $logger->{level} = normalize_level($level);
    return $logger;
}

sub logger_name {
    return $_[0]->{name};
}

sub set_name {
    my ($logger, $name) = @_;
    $logger->{name} = defined $name && length $name ? "$name" : 'dakar';
    return $logger;
}

sub logger_fields {
    return { %{ $_[0]->{fields} || {} } };
}

sub logger_field {
    my ($logger, $key, $default) = @_;
    return exists $logger->{fields}{$key} ? $logger->{fields}{$key} : $default;
}

sub logger_has_field {
    my ($logger, $key) = @_;
    return exists $logger->{fields}{$key} ? 1 : 0;
}

sub logger_capacity {
    return $_[0]->{capacity};
}

sub set_capacity {
    my ($logger, $capacity) = @_;
    $logger->{capacity} = defined $capacity && $capacity > 0 ? int($capacity) : undef;
    prune_entries($logger);
    return $logger;
}

sub with_field {
    my ($logger, $key, $value) = @_;
    $logger->{fields}{$key} = $value;
    return $logger;
}

sub with_fields {
    my ($logger, %fields) = @_;
    $logger->{fields} = { %{ $logger->{fields} || {} }, %fields };
    return $logger;
}

sub without_field {
    my ($logger, $key) = @_;
    delete $logger->{fields}{$key};
    return $logger;
}

sub clear_fields {
    my ($logger) = @_;
    $logger->{fields} = {};
    return $logger;
}

sub log_event {
    my ($logger, $level, $message, %fields) = @_;
    $level = normalize_level($level);
    return $logger unless level_enabled($level, $logger->{level});
    push @{ $logger->{entries} }, entry(
        level   => $level,
        message => $message,
        fields  => { %{ $logger->{fields} || {} }, %fields },
    );
    prune_entries($logger);
    return $logger;
}

sub log_many {
    my ($logger, $events) = @_;
    for my $event (@{ $events || [] }) {
        log_event($logger, $event->{level}, $event->{message}, %{ $event->{fields} || {} });
    }
    return $logger;
}

sub trace { return log_event($_[0], 'trace', $_[1], @_[2 .. $#_]) }
sub debug { return log_event($_[0], 'debug', $_[1], @_[2 .. $#_]) }
sub info  { return log_event($_[0], 'info',  $_[1], @_[2 .. $#_]) }
sub warn  { return log_event($_[0], 'warn',  $_[1], @_[2 .. $#_]) }
sub error { return log_event($_[0], 'error', $_[1], @_[2 .. $#_]) }
sub fatal { return log_event($_[0], 'fatal', $_[1], @_[2 .. $#_]) }

sub entries {
    return @{ $_[0]->{entries} || [] };
}

sub entry_count {
    return scalar @{ $_[0]->{entries} || [] };
}

sub last_entry {
    my ($logger) = @_;
    return undef unless entry_count($logger);
    return $logger->{entries}[-1];
}

sub clear_entries {
    my ($logger) = @_;
    $logger->{entries} = [];
    return $logger;
}

sub prune_entries {
    my ($logger) = @_;
    my $capacity = $logger->{capacity};
    return $logger unless defined $capacity && $capacity > 0;
    my $entries = $logger->{entries} || [];
    splice @{$entries}, 0, @{$entries} - $capacity if @{$entries} > $capacity;
    return $logger;
}

sub logger_snapshot {
    my ($logger) = @_;
    return logger_clone($logger);
}

sub logger_restore {
    my ($logger, $snapshot) = @_;
    $logger->{level} = $snapshot->{level};
    $logger->{name} = $snapshot->{name};
    $logger->{fields} = { %{ $snapshot->{fields} || {} } };
    $logger->{entries} = [ map { entry_to_hash($_) } @{ $snapshot->{entries} || [] } ];
    $logger->{capacity} = $snapshot->{capacity};
    return $logger;
}

sub reset_logger {
    my ($logger) = @_;
    clear_entries($logger);
    clear_fields($logger);
    return $logger;
}

1;
