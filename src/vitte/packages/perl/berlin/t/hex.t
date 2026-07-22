use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin::Buffer qw(buffer buffer_bytes);
use Vitte::City::Berlin::Hex qw(from_hex is_hex to_hex);

Test::More::is(to_hex(buffer(0, 15, 255)), '000fff', 'to_hex encodes bytes');
Test::More::is_deeply(buffer_bytes(from_hex('00 0f ff')), [0, 15, 255], 'from_hex decodes whitespace');
Test::More::ok(is_hex('aabbcc'), 'valid hex');
Test::More::ok(!is_hex('aabbc'), 'odd hex rejected');
Test::More::ok(!is_hex('zz'), 'invalid hex rejected');
Test::More::done_testing();
