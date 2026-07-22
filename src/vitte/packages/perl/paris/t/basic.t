use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use Vitte::City::Paris qw(
    args_to_hash command command_spec flag_count get_flag_count has_flag
    help_requested merge_defaults normalize_flag option option_bool option_int
    option_list options parse_args parse_config_line parse_env positionals
    quote_arg render_table require_option route_command shell_join spec_option
    split_kv subcommand unknown_options usage validate_options validate_spec
    version_requested
);

my $parsed = parse_args([qw(build --output main -vv src/main.vit --target=native --include a --include b)]);
is(command($parsed), 'build', 'command');
is(option($parsed, 'output'), 'main', 'option');
is(option($parsed, '--target'), 'native', 'long option assignment');
is_deeply(option_list($parsed, 'include'), [qw(a b)], 'option_list');
is_deeply(positionals($parsed), [qw(build src/main.vit)], 'positionals');
ok(has_flag($parsed, 'v'), 'has_flag');
is(flag_count($parsed, 'v'), 2, 'flag_count');
is(get_flag_count($parsed, 'v'), 2, 'get_flag_count');
is_deeply(options($parsed)->{include}, [qw(a b)], 'options');

is(option_int(parse_args([qw(--jobs 4)]), 'jobs', 1), 4, 'option_int');
is(option_int(parse_args([qw(--jobs nope)]), 'jobs', 1), 1, 'option_int default');
ok(option_bool(parse_args([qw(--json)]), 'json', 0), 'option_bool flag');
ok(!option_bool(parse_args([qw(--color false)]), 'color', 1), 'option_bool false');

my $workspace = parse_args([qw(workspace build --package core)]);
is(subcommand($workspace), 'build', 'subcommand');
is(route_command($workspace, { workspace => 'WORKSPACE', default => 'DEFAULT' }), 'WORKSPACE', 'route_command');

like(usage(name => 'vitte', summary => 'compiler', options => [{ flag => '--help', description => 'show help' }]), qr/Usage: vitte/, 'usage');
is(require_option($parsed, 'output'), 'main', 'require_option');
like(do { eval { require_option(parse_args([]), 'missing') }; $@ }, qr/missing required option/, 'require_option dies');

is(option(merge_defaults(parse_args([]), { profile => 'debug' }), 'profile'), 'debug', 'merge_defaults');
is(parse_env('VITTE_', { VITTE_ROOT => '/opt/vitte', OTHER => 'x' })->{root}, '/opt/vitte', 'parse_env');
is(normalize_flag('--dry-run'), 'dry_run', 'normalize_flag');
is_deeply([split_kv('--target=native')], [qw(target native)], 'split_kv');
ok(help_requested(parse_args([qw(--help)])), 'help_requested');
ok(version_requested(parse_args([qw(-V)])), 'version_requested');
is_deeply(unknown_options(parse_args([qw(--bad --good)]), [qw(good)]), ['bad'], 'unknown_options');
ok(!validate_options(parse_args([qw(--bad)]), [qw(good)])->{ok}, 'validate_options');

is(parse_config_line('key=value')->{value}, 'value', 'parse_config_line equals');
is(parse_config_line('key: value')->{key}, 'key', 'parse_config_line colon');
is(args_to_hash($parsed)->{options}{output}[0], 'main', 'args_to_hash');
like(render_table([[alpha => 'one'], [b => 'two']]), qr/alpha\s+one/, 'render_table');
is(quote_arg('abc-123'), 'abc-123', 'quote_arg simple');
is(quote_arg('a b'), q{'a b'}, 'quote_arg spaced');
is(shell_join('a b', 'c'), q{'a b' c}, 'shell_join');
is(spec_option(flag => '--dry-run')->{name}, 'dry_run', 'spec_option');
ok(validate_spec(command_spec(name => 'vitte', options => [spec_option(name => 'json')]))->{ok}, 'validate_spec');
ok(!validate_spec(command_spec(options => [spec_option(name => 'json'), spec_option(name => 'json')]))->{ok}, 'validate_spec errors');

my $literal = parse_args([qw(run -- --not-option file)]);
is_deeply(positionals($literal), [qw(run --not-option file)], 'literal args after --');

done_testing();
