use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Berlin qw(buffer crc32);

my $count = shift || 1000;
crc32(buffer(0 .. 255)) for 1 .. $count;
print "checksum $count\n";
