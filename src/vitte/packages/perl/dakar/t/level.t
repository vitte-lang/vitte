use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Level qw(level_above level_all level_below level_compare level_enabled level_known level_name level_value max_level min_level normalize_level);

Test::More::is(normalize_level('WARN'), 'warn', 'normalize upper level');
Test::More::is(level_value('fatal'), 50, 'level value');
Test::More::is(level_name(5), 'trace', 'level name');
Test::More::ok(level_known('debug'), 'known');
Test::More::is_deeply(level_all(), ['trace', 'debug', 'info', 'warn', 'error', 'fatal'], 'all levels');
Test::More::is(level_compare('info', 'warn'), -1, 'compare');
Test::More::ok(level_enabled('warn', 'info'), 'enabled');
Test::More::ok(level_above('fatal', 'error'), 'above');
Test::More::ok(level_below('trace', 'debug'), 'below');
Test::More::is(min_level('debug', 'info'), 'debug', 'min');
Test::More::is(max_level('debug', 'info'), 'info', 'max');

Test::More::done_testing();
