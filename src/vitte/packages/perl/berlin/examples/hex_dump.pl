use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Berlin qw(from_string to_hex);

print to_hex(from_string('Berlin')), "\n";
