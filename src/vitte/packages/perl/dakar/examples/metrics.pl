use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Dakar qw(count_by_level error info logger);

my $log = logger('info');
info($log, 'ready');
error($log, 'failed');
print count_by_level($log)->{error} . "\n";
