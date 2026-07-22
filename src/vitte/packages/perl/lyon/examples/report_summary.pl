use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Lyon qw(ok err report report_add report_to_text);

my $report = report();
report_add($report, ok('parse'));
report_add($report, ok('typeck'));
report_add($report, err('LYON_E_BUILD'));

print report_to_text($report), "\n";
