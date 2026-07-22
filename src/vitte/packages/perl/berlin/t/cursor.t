use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin::Buffer qw(buffer buffer_bytes);
use Vitte::City::Berlin::Cursor qw(cursor cursor_position read_u8 remaining seek write_u8);

my $buf = cursor(buffer(10, 20, 30));
Test::More::is(cursor_position($buf), 0, 'cursor starts at zero');
Test::More::is(read_u8($buf), 10, 'read_u8 reads first byte');
Test::More::is(remaining($buf), 2, 'remaining decreases');
seek($buf, 1);
write_u8($buf, 99);
Test::More::is_deeply(buffer_bytes($buf), [10, 99, 30], 'write_u8 overwrites');
seek($buf, 3);
write_u8($buf, 40);
Test::More::is_deeply(buffer_bytes($buf), [10, 99, 30, 40], 'write_u8 appends at end');
Test::More::done_testing();
