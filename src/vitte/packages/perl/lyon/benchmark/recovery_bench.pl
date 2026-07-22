use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Time::HiRes qw(time);
use Vitte::City::Lyon qw(retry unwrap);

my $start = time();
my $count = 0;
my $result = retry(sub { $count++; return $count }, 1);
printf "lyon recovery value=%d elapsed=%.6f\n", unwrap($result), time() - $start;
