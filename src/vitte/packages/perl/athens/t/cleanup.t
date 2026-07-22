use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use File::Temp qw(tempdir);
use Test::More ();

use Vitte::City::Athens::Atomic qw(write_text);
use Vitte::City::Athens::Cleanup qw(cleanup_add_dir cleanup_dry_run cleanup_plan cleanup_run);
use Vitte::City::Athens::Path qw(join_path);
use Vitte::City::Athens::Permissions qw(ensure_dir);

my $root = tempdir(CLEANUP => 1);
my $dir = join_path($root, 'nested');
my $file = join_path($dir, 'file.txt');
ensure_dir($dir);
write_text($file, 'delete me');

my $plan = cleanup_plan();
cleanup_add_dir($plan, $dir);
Test::More::is(scalar @{ $plan->{entries} }, 1, 'cleanup_add_dir updates entries');
Test::More::is(cleanup_dry_run($plan)->[0]->{path}, $dir, 'cleanup_dry_run lists paths');
Test::More::ok(cleanup_run($plan)->{ok}, 'cleanup_run reports success');
Test::More::ok(!-e $dir, 'cleanup_run removes recursively');
Test::More::done_testing();
