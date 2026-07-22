package Vitte::City::Berlin::Error;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    berlin_error
    error_code
    error_context
    error_message
    error_operation
    error_offset
    error_to_hash
    error_to_string
    error_with_context
    error_is
);

sub berlin_error {
    my ($code, $message, %context) = @_;
    return {
        package   => 'berlin',
        code      => defined $code && length $code ? $code : 'BERLIN_E_UNKNOWN',
        message   => defined $message && length $message ? $message : 'unknown Berlin error',
        context   => { %context },
        operation => $context{operation},
        offset    => $context{offset},
    };
}

sub error_code {
    return $_[0]->{code};
}

sub error_message {
    return $_[0]->{message};
}

sub error_context {
    return { %{ $_[0]->{context} || {} } };
}

sub error_operation {
    return $_[0]->{operation};
}

sub error_offset {
    return $_[0]->{offset};
}

sub error_with_context {
    my ($err, %extra) = @_;
    return berlin_error(
        error_code($err),
        error_message($err),
        %{ error_context($err) },
        %extra,
    );
}

sub error_to_hash {
    my ($err) = @_;
    return {
        package   => $err->{package},
        code      => error_code($err),
        message   => error_message($err),
        context   => error_context($err),
        operation => error_operation($err),
        offset    => error_offset($err),
    };
}

sub error_to_string {
    my ($err) = @_;
    my $text = error_code($err) . ': ' . error_message($err);
    $text .= ' at byte ' . error_offset($err) if defined error_offset($err);
    $text .= ' during ' . error_operation($err) if defined error_operation($err);
    return $text;
}

sub error_is {
    my ($err, $code) = @_;
    return defined $err && error_code($err) eq $code ? 1 : 0;
}

1;
