use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Athens::Error qw(athens_error);

my $err = athens_error('ATHENS_E_PATH', 'invalid path', path => '../x', op => 'normalize');
Test::More::is($err->{package}, 'athens', 'error records package');
Test::More::is($err->{code}, 'ATHENS_E_PATH', 'error records code');
Test::More::is($err->{message}, 'invalid path', 'error records message');
Test::More::is($err->{context}->{op}, 'normalize', 'error records context');
Test::More::done_testing();
