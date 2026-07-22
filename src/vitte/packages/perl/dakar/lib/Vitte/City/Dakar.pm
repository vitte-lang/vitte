package Vitte::City::Dakar;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(logger log_event debug info warn error entries with_field filter_level);

my %LEVEL = (debug => 10, info => 20, warn => 30, error => 40);

sub logger { return { level => $_[0] || 'info', fields => {}, entries => [] } }

sub with_field {
    my ($logger, $key, $value) = @_;
    $logger->{fields}->{$key} = $value;
    return $logger;
}

sub log_event {
    my ($logger, $level, $message, %fields) = @_;
    return $logger if $LEVEL{$level} < $LEVEL{ $logger->{level} };
    push @{ $logger->{entries} }, { level => $level, message => $message, fields => { %{ $logger->{fields} }, %fields } };
    return $logger;
}

sub debug { return log_event($_[0], 'debug', $_[1]) }
sub info { return log_event($_[0], 'info', $_[1]) }
sub warn { return log_event($_[0], 'warn', $_[1]) }
sub error { return log_event($_[0], 'error', $_[1]) }
sub entries { return @{ $_[0]->{entries} || [] } }

sub filter_level {
    my ($logger, $level) = @_;
    return [ grep { $_->{level} eq $level } entries($logger) ];
}

1;
