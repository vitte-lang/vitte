package Vitte::City::Cairo::Error;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    cairo_error
    error_code
    error_context
    error_field
    error_is
    error_message
    error_path
    error_to_hash
    error_to_string
    error_with_context
);

sub cairo_error {
    my ($code, $message, %context) = @_;
    return {
        package => 'cairo',
        code    => defined $code && length $code ? $code : 'CAIRO_E_UNKNOWN',
        message => defined $message && length $message ? $message : 'unknown Cairo error',
        context => { %context },
        field   => $context{field},
        path    => $context{path},
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

sub error_field {
    return $_[0]->{field};
}

sub error_with_context {
    my ($err, %extra) = @_;
    return cairo_error(
        error_code($err),
        error_message($err),
        %{ error_context($err) },
        %extra,
    );
}

sub error_to_hash {
    my ($err) = @_;
    return {
        package => $err->{package},
        code    => error_code($err),
        message => error_message($err),
        context => error_context($err),
        field   => error_field($err),
        path    => error_path($err),
    };
}

sub error_to_string {
    my ($err) = @_;
    my $text = error_code($err) . ': ' . error_message($err);
    $text .= ' field=' . error_field($err) if defined error_field($err);
    $text .= ' path=' . error_path($err) if defined error_path($err);
    return $text;
}

sub error_is {
    my ($err, $code) = @_;
    return defined $err && error_code($err) eq $code ? 1 : 0;
}

1;
