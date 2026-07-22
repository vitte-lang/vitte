use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo::Error qw(
    cairo_error error_code error_context error_field error_is error_message error_path
    error_to_hash error_to_string error_with_context
);

my $err = cairo_error('CAIRO_E_SCHEMA', 'invalid record', field => 'id', path => '/rows/0');
Test::More::is(error_code($err), 'CAIRO_E_SCHEMA', 'error code');
Test::More::is(error_message($err), 'invalid record', 'error message');
Test::More::is(error_context($err)->{field}, 'id', 'error context');
Test::More::is(error_field($err), 'id', 'error field');
Test::More::is(error_path($err), '/rows/0', 'error path');
Test::More::is(error_to_hash($err)->{package}, 'cairo', 'hash package');
Test::More::like(error_to_string($err), qr/field=id/, 'string includes field');
Test::More::is(error_with_context($err, source => 'test')->{context}->{source}, 'test', 'context merge');
Test::More::ok(error_is($err, 'CAIRO_E_SCHEMA'), 'error_is');

Test::More::done_testing();
