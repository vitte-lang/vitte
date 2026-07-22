package Vitte::City::Berlin::Error;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(berlin_error);

sub berlin_error {
    my ($code, $message, %context) = @_;
    return {
        package => 'berlin',
        code => defined $code ? $code : 'BERLIN_E_UNKNOWN',
        message => defined $message ? $message : 'unknown Berlin error',
        context => \%context,
    };
}

1;
