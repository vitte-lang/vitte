use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Tokyo qw(runtime spawn run_next block_on join_all defer task_count);

Test::More::is($Vitte::City::Tokyo::VERSION, '0.1.0', 'version is 0.1.0');

my $rt = runtime();
spawn($rt, 'a', sub { 1 });
defer($rt, sub { 2 });
Test::More::is(task_count($rt), 2, 'queued tasks are counted');
Test::More::is_deeply([block_on($rt)], [1, 2], 'runtime executes in order');
Test::More::is_deeply(join_all(sub { 'x' }, sub { 'y' }), ['x', 'y'], 'join_all collects values');
Test::More::is(task_count($rt), 0, 'queue is drained');

Test::More::done_testing();
