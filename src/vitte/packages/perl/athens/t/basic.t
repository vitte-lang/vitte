use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Athens qw(
    temp_name temp_path temp_file temp_dir
    cleanup_plan cleanup_add cleanup_run cleanup_dry_run
    join_path normalize_path basename dirname extension without_extension
    replace_extension is_absolute is_relative split_path path_components
    ensure_dir atomic_write read_text write_text
    manifest manifest_add_file manifest_add_dir manifest_paths manifest_size
    permissions is_readable is_writable athens_error
);

Test::More::is($Vitte::City::Athens::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::like(temp_name('unit', '.tmp'), qr/^unit-\d+-\d+-\d+\.tmp$/, 'temp_name formats stable parts');
Test::More::like(temp_path('/tmp', 'unit', '.tmp'), qr{^/tmp/unit-.*\.tmp$}, 'temp_path uses requested root');
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

my $m = manifest();
manifest_add_dir($m, 'bin');
manifest_add_file($m, 'bin/vitte', 12);
Test::More::is_deeply(manifest_paths($m), ['bin', 'bin/vitte'], 'manifest keeps entry order');
Test::More::is(manifest_size($m), 12, 'manifest_size totals files');

my $err = athens_error('ATHENS_E_TEST', 'message', path => 'x');
Test::More::is($err->{code}, 'ATHENS_E_TEST', 'athens_error records code');
Test::More::is($err->{context}->{path}, 'x', 'athens_error records context');

Test::More::ok(defined &temp_file && defined &temp_dir, 'filesystem helpers are exported');
Test::More::ok(defined &cleanup_plan && defined &cleanup_add && defined &cleanup_run && defined &cleanup_dry_run, 'cleanup helpers are exported');
Test::More::ok(defined &ensure_dir && defined &atomic_write && defined &read_text && defined &write_text, 'io helpers are exported');
Test::More::ok(defined &permissions && defined &is_readable && defined &is_writable, 'permission helpers are exported');
Test::More::done_testing();
