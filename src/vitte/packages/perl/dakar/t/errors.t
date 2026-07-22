use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Error qw(dakar_error error_code error_context error_is error_message error_operation error_to_hash error_to_string error_with_context);

my $err = dakar_error('DAKAR_E_FORMAT', 'bad format', operation => 'format');
Test::More::is(error_code($err), 'DAKAR_E_FORMAT', 'code');
Test::More::is(error_message($err), 'bad format', 'message');
Test::More::is(error_context($err)->{operation}, 'format', 'context');
Test::More::is(error_operation($err), 'format', 'operation');
Test::More::is(error_to_hash($err)->{package}, 'dakar', 'hash');
Test::More::like(error_to_string($err), qr/DAKAR_E_FORMAT/, 'string');
Test::More::is(error_with_context($err, source => 'test')->{context}->{source}, 'test', 'merge');
Test::More::ok(error_is($err, 'DAKAR_E_FORMAT'), 'is');

Test::More::done_testing();
