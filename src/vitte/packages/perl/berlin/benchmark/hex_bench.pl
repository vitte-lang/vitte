use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Berlin qw(buffer to_hex);

my $count = shift || 1000;
to_hex(buffer(0 .. 255)) for 1 .. $count;
print "hex $count\n";
