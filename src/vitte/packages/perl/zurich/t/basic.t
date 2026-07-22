use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Zurich qw(ordered_map map_put map_get map_keys set set_has set_add group_by index_by);

Test::More::is($Vitte::City::Zurich::VERSION, '0.1.0', 'version is 0.1.0');

my $m = ordered_map();
map_put($m, a => 1);
map_put($m, b => 2);
map_put($m, a => 3);
Test::More::is_deeply([map_keys($m)], ['a', 'b'], 'ordered map keeps insertion order');
Test::More::is(map_get($m, 'a'), 3, 'ordered map updates value');
Test::More::ok(set_has(set(qw(a b)), 'b'), 'set lookup works');
Test::More::is_deeply(group_by([1, 2, 3], sub { $_[0] % 2 })->{1}, [1, 3], 'group_by partitions');

Test::More::done_testing();
