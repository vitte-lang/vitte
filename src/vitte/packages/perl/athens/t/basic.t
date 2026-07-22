use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Athens qw(temp_name temp_path cleanup_plan join_path basename dirname extension);

Test::More::is($Vitte::City::Athens::VERSION, '0.1.0', 'version is 0.1.0');

my $path = temp_path('/tmp', 'unit', '.tmp');
Test::More::like($path, qr{^/tmp/unit-.*\.tmp$}, 'temp path uses prefix and suffix');
Test::More::is(join_path('/tmp/', '/a', 'b'), '/tmp/a/b', 'join_path normalizes separators');
Test::More::is(cleanup_plan('a', 'b')->{count}, 2, 'cleanup plan counts paths');
Test::More::is(basename('/a/b.txt'), 'b.txt', 'basename extracts file');
Test::More::is(extension('/a/b.txt'), 'txt', 'extension extracts suffix');

Test::More::done_testing();
