use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon::Error qw(error_code error_is error_to_string error_with_cause lyon_error);

my $err = error_with_cause(lyon_error('LYON_E_TOP', 'top'), lyon_error('LYON_E_CAUSE', 'cause'));
Test::More::is(error_code($err), 'LYON_E_TOP', 'code');
Test::More::ok(error_is($err, 'LYON_E_TOP'), 'is');
Test::More::like(error_to_string($err), qr/LYON_E_CAUSE/, 'cause rendered');
Test::More::done_testing();
