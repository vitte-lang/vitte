use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Time::HiRes qw(time);
use Vitte::City::Paris qw(parse_args);

my $start = time();
for (1 .. 10_000) {
    parse_args([qw(build --output main -vv src/main.vit --target=native)]);
}
printf "paris parse iterations=10000 elapsed=%.6f\n", time() - $start;
