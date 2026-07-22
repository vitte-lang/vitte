use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon::Option qw(is_none is_some none option_filter option_from_value option_map option_or some unwrap_option);

Test::More::ok(is_some(some(1)), 'some');
Test::More::ok(is_none(none()), 'none');
Test::More::is(unwrap_option(option_map(some(1), sub { $_[0] + 1 })), 2, 'map');
Test::More::is(option_or(none(), 'x'), 'x', 'or');
Test::More::ok(is_none(option_filter(some(1), sub { 0 })), 'filter');
Test::More::ok(is_none(option_from_value(undef)), 'from undef');
Test::More::done_testing();
