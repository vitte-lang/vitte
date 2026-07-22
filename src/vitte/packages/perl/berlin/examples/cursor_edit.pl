use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Berlin qw(buffer seek to_hex write_u8);

my $buf = buffer(1, 2, 3);
seek($buf, 1);
write_u8($buf, 99);
print to_hex($buf), "\n";
