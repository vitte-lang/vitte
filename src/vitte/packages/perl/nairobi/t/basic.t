use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Nairobi qw(env_get env_has path_list path_join path_normalize with_env program_name);

Test::More::is($Vitte::City::Nairobi::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is_deeply(path_list('a:b'), ['a', 'b'], 'path_list splits');
Test::More::is(path_join('/tmp/', '/x'), '/tmp/x', 'path_join normalizes slashes');
Test::More::is(path_normalize('/a/./b/../c'), '/a/c', 'path_normalize resolves parts');
Test::More::is(with_env('VITTE_PERL_TEST', 'ok', sub { env_get('VITTE_PERL_TEST') }), 'ok', 'with_env scopes value');
Test::More::is(program_name('/bin/vitte'), 'vitte', 'program_name extracts basename');

Test::More::done_testing();
