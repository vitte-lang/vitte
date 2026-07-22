use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin::Base64 qw(base64_decode base64_encode);
use Vitte::City::Berlin::Buffer qw(from_string to_string);

Test::More::is(base64_encode(from_string('Man')), 'TWFu', 'base64 no padding');
Test::More::is(base64_encode(from_string('Ma')), 'TWE=', 'base64 one padding');
Test::More::is(base64_encode(from_string('M')), 'TQ==', 'base64 two padding');
Test::More::is(to_string(base64_decode('QmVybGlu')), 'Berlin', 'base64 decode');
Test::More::done_testing();
