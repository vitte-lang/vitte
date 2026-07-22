use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use Vitte::City::Paris qw(quote_arg shell_join render_table);

is(quote_arg('plain'), 'plain', 'plain arg unquoted');
is(quote_arg('two words'), q{'two words'}, 'space is quoted');
is(quote_arg(q{can't}), q{'can'"'"'t'}, 'single quote is shell-safe');
is(shell_join('two words', 'plain', 'x/y'), q{'two words' plain x/y}, 'shell_join');
is(render_table([]), '', 'empty table');
like(render_table([[one => 1], [three => 3]]), qr/three\s+3/, 'table output');

done_testing();
