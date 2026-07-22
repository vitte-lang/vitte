use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin qw(buffer from_string append length_of slice to_hex from_hex concat_buffers);

Test::More::is($Vitte::City::Berlin::VERSION, '0.1.0', 'version is 0.1.0');

my $buf = buffer(65, 66);
append($buf, 67);
Test::More::is(length_of($buf), 3, 'length tracks appended bytes');
Test::More::is(to_hex($buf), '414243', 'hex encoding is stable');
Test::More::is(to_hex(slice($buf, 1, 2)), '4243', 'slice copies byte range');
Test::More::is(to_hex(concat_buffers(from_hex('00ff'), from_string('A'))), '00ff41', 'buffers concatenate');

Test::More::done_testing();
