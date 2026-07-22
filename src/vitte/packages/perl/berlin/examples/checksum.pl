use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Berlin qw(crc32 from_string);

printf "%08x\n", crc32(from_string('Berlin'));
