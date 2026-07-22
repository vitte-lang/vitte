use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Athens qw(manifest manifest_add_file manifest_size);

my $count = shift || 1000;
my $m = manifest();
manifest_add_file($m, "file-$_.txt", $_) for 1 .. $count;
print manifest_size($m), "\n";
