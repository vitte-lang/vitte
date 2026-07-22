use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Athens qw(cleanup_add cleanup_plan cleanup_size join_path normalize_path replace_extension split_path);

Test::More::is(normalize_path('/'), '/', 'root path stays root');
Test::More::is(normalize_path(''), '.', 'empty path normalizes to dot');
Test::More::is(join_path('/tmp', '', undef, 'x'), '/tmp/x', 'join_path ignores empty parts');
Test::More::is(replace_extension('archive', 'tar'), 'archive.tar', 'replace_extension adds missing extension');
Test::More::is_deeply(split_path('/'), [], 'split_path root has no components');
my $plan = cleanup_plan();
cleanup_add($plan, 'file', 'a', { ignore_missing => 1 });
cleanup_add($plan, 'file', 'b', { ignore_missing => 1 });
cleanup_add($plan, 'file', 'c', { ignore_missing => 1 });
Test::More::is(cleanup_size($plan), 3, 'cleanup count remains stable');
Test::More::done_testing();
