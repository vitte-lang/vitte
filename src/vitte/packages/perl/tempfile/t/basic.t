use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Tempfile qw(temp_name temp_path cleanup_plan);

Test::More::is($Vitte::Crates::Tempfile::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { temp_path("/tmp", "x") eq "/tmp/x" }, 'basic behavior works');
