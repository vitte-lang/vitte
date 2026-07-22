use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lima qw(map_list filter_list fold_list chunks zip take unique flatten);

Test::More::is($Vitte::City::Lima::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is_deeply(map_list([1, 2], sub { $_[0] * 2 }), [2, 4], 'map_list transforms');
Test::More::is_deeply(filter_list([1, 2, 3], sub { $_[0] > 1 }), [2, 3], 'filter_list filters');
Test::More::is(fold_list([1, 2, 3], 0, sub { $_[0] + $_[1] }), 6, 'fold_list reduces');
Test::More::is_deeply(chunks([1, 2, 3], 2), [[1, 2], [3]], 'chunks split input');
Test::More::is_deeply(unique([1, 1, 2]), [1, 2], 'unique removes duplicates');

Test::More::done_testing();
