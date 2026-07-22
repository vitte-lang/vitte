use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon::Result qw(and_then err is_err is_ok map_error map_result ok result_from_eval unwrap unwrap_err unwrap_or);

Test::More::ok(is_ok(ok(1)), 'ok result');
Test::More::ok(is_err(err('e')), 'err result');
Test::More::is(unwrap(map_result(ok(1), sub { $_[0] + 1 })), 2, 'map result');
Test::More::is(unwrap_err(map_error(err('e'), sub { uc $_[0] })), 'E', 'map error');
Test::More::is(unwrap(and_then(ok(1), sub { ok($_[0] + 2) })), 3, 'and then');
Test::More::is(unwrap_or(err('e'), 9), 9, 'unwrap_or');
Test::More::ok(is_err(result_from_eval(sub { die 'boom' })), 'eval error');
Test::More::done_testing();
