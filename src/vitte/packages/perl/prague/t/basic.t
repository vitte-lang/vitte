use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Prague qw(parse_config emit_config get_config set_config merge_config require_config);

Test::More::is($Vitte::City::Prague::VERSION, '0.1.0', 'version is 0.1.0');

my $cfg = parse_config("name=vitte\n#x\nthreads = 4\n");
Test::More::is(get_config($cfg, 'name'), 'vitte', 'config parses value');
set_config($cfg, 'mode', 'strict');
Test::More::is(require_config($cfg, 'mode'), 'strict', 'config sets value');
Test::More::is(merge_config($cfg, { threads => 8 })->{threads}, 8, 'overlay wins');
Test::More::like(emit_config({ b => 2, a => 1 }), qr/^a=1\nb=2\n$/, 'emit_config sorts keys');

Test::More::done_testing();
