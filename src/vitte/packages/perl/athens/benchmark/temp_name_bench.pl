use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Athens qw(temp_name);

my $count = shift || 1000;
temp_name('bench', '.tmp') for 1 .. $count;
print "temp_name $count\n";
