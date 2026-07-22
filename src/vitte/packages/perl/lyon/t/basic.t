use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon qw(ok err is_ok is_err value error map_ok map_err and_then unwrap_or);

Test::More::is($Vitte::City::Lyon::VERSION, '0.1.0', 'version is 0.1.0');

my $res = and_then(map_ok(ok(10), sub { $_[0] + 5 }), sub { ok($_[0] * 2) });
Test::More::is(value($res), 30, 'ok pipeline maps values');
Test::More::ok(is_err(err('boom')), 'error state is visible');
Test::More::is(error(map_err(err('boom'), sub { 'ctx: ' . $_[0] })), 'ctx: boom', 'error mapper rewrites message');
Test::More::is(unwrap_or(err('missing'), 7), 7, 'fallback works');

Test::More::done_testing();
