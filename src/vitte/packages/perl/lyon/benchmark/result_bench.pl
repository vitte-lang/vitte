use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Time::HiRes qw(time);
use Vitte::City::Lyon qw(ok map_result unwrap);

my $start = time();
my $result = ok(0);
for (1 .. 10_000) {
    $result = map_result($result, sub { $_[0] + 1 });
}
printf "lyon result map count=%d elapsed=%.6f\n", unwrap($result), time() - $start;
