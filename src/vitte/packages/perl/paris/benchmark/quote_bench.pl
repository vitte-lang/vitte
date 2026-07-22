use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Time::HiRes qw(time);
use Vitte::City::Paris qw(shell_join);

my $start = time();
for (1 .. 10_000) {
    shell_join("vitte", "build", "file with spaces.vit", "-o", "build/main");
}
printf "paris quote iterations=10000 elapsed=%.6f\n", time() - $start;
