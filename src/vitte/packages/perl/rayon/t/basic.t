use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Rayon qw(join par_map par_sum);

Test::More::is($Vitte::Crates::Rayon::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { par_sum([1,2,3]) == 6 }, 'basic behavior works');
