use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin::Error qw(berlin_error);

my $err = berlin_error('BERLIN_E_HEX', 'invalid hex', input => 'zz');
Test::More::is($err->{package}, 'berlin', 'error package');
Test::More::is($err->{code}, 'BERLIN_E_HEX', 'error code');
Test::More::is($err->{context}->{input}, 'zz', 'error context');
Test::More::done_testing();
