use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin qw(buffer from_hex is_hex slice to_hex);

Test::More::is(to_hex(buffer()), '', 'empty buffer hex is empty');
Test::More::is(to_hex(slice(buffer(1, 2), 0, 0)), '', 'zero length slice is empty');
Test::More::ok(is_hex(''), 'empty hex is valid');
Test::More::is(to_hex(from_hex('')), '', 'empty hex decodes');
Test::More::done_testing();
