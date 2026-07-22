use strict;
use warnings;
use Test::More ();

use Vitte::City::Berlin qw(from_string to_hex);

Test::More::ok($INC{'Vitte/City/Berlin.pm'}, 'loaded through PERL5LIB or local lib');
Test::More::is(to_hex(from_string('A')), '41', 'API works after load');
Test::More::done_testing();
