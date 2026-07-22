use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Athens qw(atomic_write read_text);

my $path = '/tmp/athens-atomic-example.txt';
atomic_write($path, "written\n");
print read_text($path);
