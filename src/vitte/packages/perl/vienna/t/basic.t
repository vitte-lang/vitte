use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Vienna qw(sum mean median minmax clamp lerp percentile variance);

Test::More::is($Vitte::City::Vienna::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is(sum([1, 2, 3]), 6, 'sum adds');
Test::More::is(mean([2, 4]), 3, 'mean averages');
Test::More::is(median([3, 1, 2]), 2, 'median sorts');
my ($lo, $hi) = minmax([4, 1, 9]);
Test::More::is("$lo:$hi", '1:9', 'minmax returns bounds');
Test::More::is(clamp(12, 0, 10), 10, 'clamp caps high');
Test::More::is(lerp(10, 20, 0.5), 15, 'lerp interpolates');

Test::More::done_testing();
