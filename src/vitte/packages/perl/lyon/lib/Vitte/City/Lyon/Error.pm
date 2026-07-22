package Vitte::City::Lyon::Error;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    error_cause
    error_code
    error_context
    error_is
    error_message
    error_path
    error_to_hash
    error_to_string
    error_with_cause
    error_with_context
    lyon_error
);

sub lyon_error {
    my ($code, $message, %context) = @_;
    return {
        package => 'lyon',
        code    => defined $code && length $code ? $code : 'LYON_E_UNKNOWN',
        message => defined $message && length $message ? $message : 'unknown Lyon error',
        context => { %context },
        path    => $context{path},
        cause   => $context{cause},
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

sub error_path {
    return $_[0]->{path};
}

sub error_cause {
    return $_[0]->{cause};
}

sub error_with_context {
    my ($error, %extra) = @_;
    return lyon_error(
        error_code($error),
        error_message($error),
        %{ error_context($error) },
        %extra,
    );
}

sub error_with_cause {
    my ($error, $cause) = @_;
    return error_with_context($error, cause => $cause);
}

sub error_to_hash {
    my ($error) = @_;
    return {
        package => $error->{package},
        code    => error_code($error),
        message => error_message($error),
        context => error_context($error),
        path    => error_path($error),
        cause   => error_cause($error),
    };
}

sub error_to_string {
    my ($error) = @_;
    my $text = error_code($error) . ': ' . error_message($error);
    $text .= ' at ' . error_path($error) if defined error_path($error);
    $text .= ' caused by ' . error_to_string(error_cause($error)) if ref(error_cause($error)) eq 'HASH';
    return $text;
}

sub error_is {
    my ($error, $code) = @_;
    return defined $error && error_code($error) eq $code ? 1 : 0;
}

1;
