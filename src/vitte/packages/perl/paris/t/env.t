use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use Vitte::City::Paris qw(parse_env parse_config_line merge_defaults parse_args option);

is_deeply(
    parse_env('VITTE_', { VITTE_ROOT => '/opt/vitte', VITTE_CACHE_DIR => '/tmp/cache', PATH => '/bin' }),
    { root => '/opt/vitte', cache_dir => '/tmp/cache' },
    'parse_env filters and normalizes'
);

is_deeply(parse_config_line('profile = release'), { key => 'profile', value => 'release' }, 'parse_config_line trims');
is(parse_config_line(' # comment only'), undef, 'parse_config_line ignores comments');
is(option(merge_defaults(parse_args([qw(--profile debug)]), { profile => 'release' }), 'profile'), 'debug', 'merge_defaults keeps explicit value');

done_testing();
