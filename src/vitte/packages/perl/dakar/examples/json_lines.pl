use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Dakar qw(entries error format_lines format_json logger);

my $log = logger('debug');
error($log, 'failed', code => 'E_TEST');
print format_lines([ entries($log) ], \&format_json) . "\n";
