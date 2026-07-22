package Vitte::City::Athens::Error;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(athens_error);

sub athens_error {
    my ($code, $message, %context) = @_;
    return {
        package => 'athens',
        code => defined $code ? $code : 'ATHENS_E_UNKNOWN',
        message => defined $message ? $message : 'unknown Athens error',
        context => \%context,
    };
}

1;
