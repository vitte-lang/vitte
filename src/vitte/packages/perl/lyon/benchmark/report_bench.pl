use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Time::HiRes qw(time);
use Vitte::City::Lyon qw(ok report report_add report_summary);

my $start = time();
my $report = report();
for (1 .. 10_000) {
    report_add($report, ok($_));
}
my $summary = report_summary($report);
printf "lyon report total=%d elapsed=%.6f\n", $summary->{total}, time() - $start;
