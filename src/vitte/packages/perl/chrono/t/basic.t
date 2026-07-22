use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Chrono qw(duration seconds millis format_epoch);

Test::More::is($Vitte::Crates::Chrono::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { duration(seconds=>2,millis=>5) == 2005 }, 'basic behavior works');
