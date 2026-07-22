use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Berlin qw(pack_u32_be to_hex);

print to_hex(pack_u32_be(0x12345678)), "\n";
