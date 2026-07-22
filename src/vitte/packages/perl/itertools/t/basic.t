use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Itertools qw(map_list filter_list fold_list chunks);

Test::More::is($Vitte::Crates::Itertools::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { fold_list([1,2,3],0,sub{$_[0]+$_[1]}) == 6 }, 'basic behavior works');
