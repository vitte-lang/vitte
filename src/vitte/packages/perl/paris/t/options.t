use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use Vitte::City::Paris qw(parse_args has_flag option option_list option_bool option_int options);

my $p = parse_args([qw(-abc -oout --mode release --mode debug --threads=8 --trace)]);
ok(has_flag($p, 'a'), 'short flag a');
ok(has_flag($p, 'b'), 'short flag b');
ok(has_flag($p, 'c'), 'short flag c');
is(option($p, 'o'), 'out', 'short attached option');
is(option($p, 'mode'), 'debug', 'last option wins');
is_deeply(option_list($p, 'mode'), [qw(release debug)], 'all option values retained');
is(option_int($p, 'threads', 1), 8, 'integer option');
ok(option_bool($p, 'trace', 0), 'boolean flag');
is_deeply([sort keys %{ options($p) }], [qw(mode o threads)], 'options keys stable');

done_testing();
