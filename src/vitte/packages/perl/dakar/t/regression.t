use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar qw(entry_count filter_level format_lines logger normalize_level set_capacity trace);

Test::More::is(normalize_level(undef), 'info', 'undef level defaults to info');
my $log = logger('fatal');
trace($log, 'hidden');
Test::More::is(entry_count($log), 0, 'trace below fatal is hidden');
set_capacity($log, 0);
Test::More::is(entry_count($log), 0, 'zero capacity means unbounded and does not crash');
Test::More::is_deeply(filter_level([], 'info'), [], 'empty filter stable');
Test::More::is(format_lines([]), '', 'empty lines stable');

Test::More::done_testing();
