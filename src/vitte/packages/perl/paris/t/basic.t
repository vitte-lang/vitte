use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Paris qw(parse_args has_flag option positionals usage require_option merge_defaults);

Test::More::is($Vitte::City::Paris::VERSION, '0.1.0', 'version is 0.1.0');

my $args = parse_args('--out=bin', '--target', 'vm', '-v', 'main.vit');
Test::More::ok(has_flag($args, 'v'), 'short flag is parsed');
Test::More::is(option($args, 'target'), 'vm', 'long option value is parsed');
Test::More::is_deeply([positionals($args)], ['main.vit'], 'positionals are retained');
Test::More::is(option(merge_defaults($args, { mode => 'check' }), 'mode'), 'check', 'defaults merge');
Test::More::like(usage('vitte'), qr/^Usage:/, 'usage renders');

Test::More::done_testing();
