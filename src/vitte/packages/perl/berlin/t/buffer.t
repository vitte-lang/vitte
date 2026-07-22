use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin::Buffer qw(append buffer buffer_bytes byte_at clear from_string is_empty length_of resize reverse_bytes slice to_string);

my $buf = buffer(1, 2, 300);
Test::More::is_deeply(buffer_bytes($buf), [1, 2, 44], 'buffer masks bytes');
append($buf, 4);
Test::More::is(length_of($buf), 4, 'append grows buffer');
Test::More::is(byte_at($buf, 2), 44, 'byte_at reads masked byte');
Test::More::is_deeply(buffer_bytes(slice($buf, 1, 2)), [2, 44], 'slice copies range');
resize($buf, 2);
Test::More::is_deeply(buffer_bytes($buf), [1, 2], 'resize shrinks');
resize($buf, 4, 9);
Test::More::is_deeply(buffer_bytes($buf), [1, 2, 9, 9], 'resize grows');
Test::More::is(to_string(from_string('xy')), 'xy', 'string roundtrip');
Test::More::is_deeply(buffer_bytes(reverse_bytes(buffer(1, 2, 3))), [3, 2, 1], 'reverse bytes');
clear($buf);
Test::More::ok(is_empty($buf), 'clear empties');
Test::More::done_testing();
