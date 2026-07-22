use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use File::Temp qw(tempdir);
use Test::More ();

use Vitte::City::Athens qw(
    temp_name temp_path temp_file temp_dir temp_scope temp_counter temp_reset_counter temp_file_pair temp_dir_plan
    cleanup_plan cleanup_add cleanup_add_file cleanup_add_dir cleanup_add_callback cleanup_run cleanup_dry_run cleanup_clear cleanup_size cleanup_is_empty cleanup_errors
    join_path normalize_path basename dirname extension without_extension replace_extension is_absolute is_relative split_path path_components path_stem path_depth path_is_root path_has_extension relative_path resolve_path common_prefix change_basename
    ensure_dir permissions is_readable is_writable is_executable permission_bits chmod_path file_owner_id file_group_id file_mode_summary
    atomic_write atomic_write_bytes read_text read_bytes write_text write_bytes
    manifest manifest_add_file manifest_add_dir manifest_add_entry manifest_paths manifest_size manifest_count manifest_find manifest_remove manifest_sort manifest_to_text manifest_to_hash
    athens_error error_code error_message error_context error_path error_operation error_with_context error_to_string error_to_hash error_is
);

Test::More::is($Vitte::City::Athens::VERSION, '0.1.0', 'version is 0.1.0');

my $root = tempdir(CLEANUP => 1);

temp_reset_counter();
Test::More::is(temp_counter(), 0, 'temp_reset_counter resets sequence');
Test::More::like(temp_name('unit', '.tmp'), qr/^unit-\d+-\d+-\d+\.tmp$/, 'temp_name formats stable parts');
Test::More::is(temp_counter(), 1, 'temp_counter increments');
Test::More::like(temp_path($root, 'unit', '.tmp'), qr{/unit-.*\.tmp$}, 'temp_path uses requested root');

my $dir = temp_dir($root, 'unit-dir');
Test::More::ok(-d $dir, 'temp_dir creates directory');
my $file = temp_file($root, 'unit-file', '.txt', 'alpha');
Test::More::is(read_text($file), 'alpha', 'temp_file writes content');
my ($left, $right) = temp_file_pair($root, 'pair');
Test::More::ok(-f $left && -f $right, 'temp_file_pair creates two files');
my $scope = temp_scope($root, 'scope');
Test::More::ok(-d $scope->{root}, 'temp_scope creates scoped root');
my $dir_plan = temp_dir_plan($root, 'planned');
Test::More::ok(-d $dir_plan->{root} && -f $dir_plan->{file}, 'temp_dir_plan creates root and payload');

Test::More::is(join_path('/tmp/', '/a', 'b'), '/tmp/a/b', 'join_path normalizes separators');
Test::More::is(normalize_path('/a/./b/../c'), '/a/c', 'normalize_path resolves dot parts');
Test::More::is(basename('/a/b.txt'), 'b.txt', 'basename extracts file');
Test::More::is(dirname('/a/b.txt'), '/a', 'dirname extracts parent');
Test::More::is(extension('/a/b.txt'), 'txt', 'extension extracts suffix');
Test::More::is(without_extension('/a/b.txt'), '/a/b', 'without_extension strips suffix');
Test::More::is(replace_extension('/a/b.txt', 'log'), '/a/b.log', 'replace_extension rewrites suffix');
Test::More::ok(is_absolute('/tmp'), 'absolute path detected');
Test::More::ok(is_relative('tmp/file'), 'relative path detected');
Test::More::is_deeply(split_path('/a/b'), ['a', 'b'], 'split_path returns components');
Test::More::is_deeply(path_components('/a/b'), ['a', 'b'], 'path_components aliases split_path');
Test::More::is(path_stem('/a/b.txt'), 'b', 'path_stem strips extension');
Test::More::is(path_depth('/a/b/c'), 3, 'path_depth counts components');
Test::More::ok(path_is_root('/'), 'path_is_root detects root');
Test::More::ok(path_has_extension('/a/b.txt', 'txt'), 'path_has_extension checks suffix');
Test::More::is(relative_path('/a/b', '/a/c/d'), '../c/d', 'relative_path computes traversal');
Test::More::is(resolve_path('/a/b', '../c'), '/a/c', 'resolve_path resolves relative path');
Test::More::is(common_prefix('/a/b/c', '/a/b/d'), 'a/b', 'common_prefix finds shared components');
Test::More::is(change_basename('/a/b.txt', 'c.txt'), '/a/c.txt', 'change_basename replaces final path');

ensure_dir(join_path($root, 'perms'));
write_text(join_path($root, 'perms', 'file.txt'), 'data');
my $perm_file = join_path($root, 'perms', 'file.txt');
Test::More::ok(is_readable($perm_file), 'file is readable');
Test::More::ok(is_writable($perm_file), 'file is writable');
Test::More::ok(!is_executable($perm_file) || is_executable($perm_file) == 1, 'is_executable returns boolean');
Test::More::like(permissions($perm_file), qr/^[0-7]{4}$/, 'permissions returns octal string');
Test::More::ok(defined permission_bits($perm_file), 'permission_bits returns numeric bits');
Test::More::like(chmod_path($perm_file, 0644), qr/^[0-7]{4}$/, 'chmod_path changes mode');
Test::More::ok(defined file_owner_id($perm_file), 'file_owner_id returns uid');
Test::More::ok(defined file_group_id($perm_file), 'file_group_id returns gid');
Test::More::is(file_mode_summary($perm_file)->{readable}, 1, 'file_mode_summary returns probes');

my $atomic = join_path($root, 'atomic.txt');
Test::More::is(write_text($atomic, 'one'), 3, 'write_text returns byte length');
Test::More::is(read_text($atomic), 'one', 'read_text reads content');
Test::More::is(write_bytes($atomic, "two"), 3, 'write_bytes returns byte length');
Test::More::is(read_bytes($atomic), 'two', 'read_bytes reads bytes');
Test::More::is(atomic_write($atomic, 'three'), 5, 'atomic_write replaces content');
Test::More::is(atomic_write_bytes($atomic, 'four'), 4, 'atomic_write_bytes replaces content');

my $m = manifest();
manifest_add_dir($m, 'bin');
manifest_add_file($m, 'bin/vitte', 12, '0755');
manifest_add_entry($m, type => 'file', path => 'share/doc.txt', size => 5);
Test::More::is_deeply(manifest_paths($m), ['bin', 'bin/vitte', 'share/doc.txt'], 'manifest keeps entry order');
Test::More::is(manifest_size($m), 17, 'manifest_size totals files');
Test::More::is(manifest_count($m), 3, 'manifest_count totals entries');
Test::More::is(manifest_find($m, 'bin/vitte')->{mode}, '0755', 'manifest_find returns copy');
Test::More::is(manifest_remove($m, 'missing'), 0, 'manifest_remove reports zero');
manifest_sort($m);
Test::More::like(manifest_to_text($m), qr/bin\/vitte/, 'manifest_to_text renders entries');
Test::More::is(manifest_to_hash($m)->{count}, manifest_count($m), 'manifest_to_hash summarizes count');

my $plan = cleanup_plan({ order => 'fifo' });
Test::More::ok(cleanup_is_empty($plan), 'cleanup_is_empty detects empty plan');
cleanup_add_file($plan, join_path($root, 'no-such-file'), { ignore_missing => 1 });
cleanup_add_dir($plan, join_path($root, 'no-such-dir'), { ignore_missing => 1 });
cleanup_add_callback($plan, sub { return 'done' }, { label => 'callback' });
cleanup_add($plan, 'file', join_path($root, 'no-such-file-2'), { ignore_missing => 1 });
Test::More::is(cleanup_size($plan), 4, 'cleanup_size counts entries');
Test::More::is(scalar @{ cleanup_dry_run($plan) }, 4, 'cleanup_dry_run previews entries');
Test::More::ok(cleanup_run($plan)->{ok}, 'cleanup_run reports success');
Test::More::is_deeply(cleanup_errors($plan), [], 'cleanup_errors empty after success');
Test::More::is(cleanup_clear($plan), 0, 'cleanup_clear reports remaining entries after run');

my $err = athens_error('ATHENS_E_TEST', 'message', path => 'x', operation => 'op');
Test::More::is(error_code($err), 'ATHENS_E_TEST', 'error_code records code');
Test::More::is(error_message($err), 'message', 'error_message records message');
Test::More::is(error_path($err), 'x', 'error_path records path');
Test::More::is(error_operation($err), 'op', 'error_operation records operation');
Test::More::is(error_context($err)->{path}, 'x', 'error_context copies context');
Test::More::ok(error_is($err, 'ATHENS_E_TEST'), 'error_is matches code');
Test::More::like(error_to_string($err), qr/ATHENS_E_TEST/, 'error_to_string includes code');
Test::More::is(error_to_hash(error_with_context($err, extra => 1))->{context}->{extra}, 1, 'error_with_context merges context');

Test::More::done_testing();
