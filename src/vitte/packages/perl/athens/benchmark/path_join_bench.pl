use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Athens qw(join_path);

my $count = shift || 1000;
join_path('/tmp/', '/athens', 'file.txt') for 1 .. $count;
print "path_join $count\n";
