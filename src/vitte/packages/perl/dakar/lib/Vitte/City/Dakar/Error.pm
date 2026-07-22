package Vitte::City::Dakar::Error;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    dakar_error
    error_code
    error_context
    error_is
    error_message
    error_operation
    error_to_hash
    error_to_string
    error_with_context
);

sub dakar_error {
    my ($code, $message, %context) = @_;
    return {
        package   => 'dakar',
        code      => defined $code && length $code ? $code : 'DAKAR_E_UNKNOWN',
        message   => defined $message && length $message ? $message : 'unknown Dakar error',
        context   => { %context },
        operation => $context{operation},
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

sub error_with_context {
    my ($err, %extra) = @_;
    return dakar_error(
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
    };
}

sub error_to_string {
    my ($err) = @_;
    my $text = error_code($err) . ': ' . error_message($err);
    $text .= ' during ' . error_operation($err) if defined error_operation($err);
    return $text;
}

sub error_is {
    my ($err, $code) = @_;
    return defined $err && error_code($err) eq $code ? 1 : 0;
}

1;
