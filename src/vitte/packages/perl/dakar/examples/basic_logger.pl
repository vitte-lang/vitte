use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Dakar qw(format_lines info logger warn entries with_field);

my $log = with_field(logger('info'), component => 'compiler');
info($log, 'ready');
warn($log, 'slow');
print format_lines([ entries($log) ]) . "\n";
