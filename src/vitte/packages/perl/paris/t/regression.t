use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use Vitte::City::Paris qw(parse_args option option_bool positionals normalize_flag split_kv quote_arg validate_spec command_spec spec_option);

is(option(parse_args([qw(--zero 0)]), 'zero', 'x'), '0', 'zero string is preserved');
is(option_bool(parse_args([qw(--feature off)]), 'feature', 1), 0, 'off maps to false');
is_deeply(positionals(parse_args([qw(-- --flag value)])), [qw(--flag value)], 'dashdash keeps literal args');
is(normalize_flag('-dry-run'), 'dry_run', 'single dash long spelling');
is_deeply([split_kv('--empty=')], ['empty', ''], 'empty assignment preserved');
is(quote_arg(''), q{''}, 'empty arg quoted');
ok(!validate_spec(command_spec(name => 'x', options => [spec_option(name => 'a'), spec_option(flag => '--a')]))->{ok}, 'duplicate normalized option rejected');

done_testing();
