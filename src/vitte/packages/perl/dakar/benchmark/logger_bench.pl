use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Dakar qw(entry_count info logger);

my $log = logger('info');
for (1 .. 1000) {
    info($log, "message $_", index => $_);
}
print entry_count($log) . "\n";
