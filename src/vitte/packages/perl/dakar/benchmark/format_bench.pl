use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Dakar qw(entries format_lines info logger);

my $log = logger('info');
info($log, "message $_") for 1 .. 500;
print length(format_lines([ entries($log) ])) . "\n";
