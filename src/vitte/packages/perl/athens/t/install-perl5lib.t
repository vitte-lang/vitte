use strict;
use warnings;
use Test::More ();

use Vitte::City::Athens qw(join_path temp_name);

Test::More::ok($INC{'Vitte/City/Athens.pm'}, 'loaded through PERL5LIB or local lib');
Test::More::is(join_path('/a', 'b'), '/a/b', 'loaded API works');
Test::More::like(temp_name('install', '.tmp'), qr/^install-.*\.tmp$/, 'temp API works after load');
Test::More::done_testing();
