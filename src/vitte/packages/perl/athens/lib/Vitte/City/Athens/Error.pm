package Vitte::City::Athens::Error;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    athens_error
    error_code
    error_message
    error_context
    error_path
    error_operation
    error_with_context
    error_to_string
    error_to_hash
    error_is
);

sub athens_error {
    my ($code, $message, %context) = @_;
    return {
        package => 'athens',
        code => defined $code && length $code ? $code : 'ATHENS_E_UNKNOWN',
        message => defined $message && length $message ? $message : 'unknown Athens error',
        context => { %context },
    };
}

sub error_code {
    my ($error) = @_;
    return $error->{code};
}

sub error_message {
    my ($error) = @_;
    return $error->{message};
}

sub error_context {
    my ($error) = @_;
    return { %{ $error->{context} || {} } };
}

sub error_path {
    my ($error) = @_;
    return $error->{context}->{path};
}

sub error_operation {
    my ($error) = @_;
    return $error->{context}->{operation};
}

sub error_with_context {
    my ($error, %context) = @_;
    return athens_error(
        $error->{code},
        $error->{message},
        %{ $error->{context} || {} },
        %context,
    );
}

sub error_to_string {
    my ($error) = @_;
    my $path = error_path($error);
    my $operation = error_operation($error);
    my $suffix = '';
    $suffix .= " operation=$operation" if defined $operation && length $operation;
    $suffix .= " path=$path" if defined $path && length $path;
    return $error->{code} . ': ' . $error->{message} . $suffix;
}

sub error_to_hash {
    my ($error) = @_;
    return {
        package => $error->{package},
        code => $error->{code},
        message => $error->{message},
        context => error_context($error),
    };
}

sub error_is {
    my ($error, $code) = @_;
    return defined $error && defined $code && $error->{code} eq $code ? 1 : 0;
}

1;
