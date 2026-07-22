use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Berlin qw(append buffer);

my $count = shift || 1000;
my $buf = buffer();
append($buf, $_) for 1 .. $count;
print "buffer $count\n";
