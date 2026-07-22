use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Dakar qw(error filter_min_level info logger);

my $log = logger('info');
for (1 .. 500) {
    $_ % 10 ? info($log, "ok $_") : error($log, "bad $_");
}
print scalar @{ filter_min_level($log, 'error') } . "\n";
